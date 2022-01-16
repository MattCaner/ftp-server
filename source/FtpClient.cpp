#include"FtpClient.hpp"
#include"utils.h"
using namespace std::placeholders;

FtpClient::FtpClient(int myId, asio::ip::tcp::socket&& socket, asio::io_context& context, std::map<std::string,std::string>& users, Logger& logger, std::vector<std::unique_ptr<FtpClient>>& clientsVector): _myId(myId), _socket(std::move(socket)), _context(context),_pwd("."),_users(users),_logger(logger),_clientsVector(clientsVector){
    asio::async_read_until(_socket,_buff,"\r\n",std::bind(&FtpClient::_onread,this,_1,_2));
}

void FtpClient::_onread(const asio::error_code& e, std::size_t size){
    if(e){
        _socket.close();
        std::cout << e.message() <<std::endl;
        return;
    }
    std::istream is(&_buff);
    std::string line;
    line = line.substr(0, line.size()-2);
    while (std::getline(is, line)) {
        _logger.log("INFO","Command came: "+line);
        std::cout << "Read: " << line << std::endl;
        if(line.rfind("USER", 0) == 0){
            std::string arg = line.substr(line.find(" ") + 1);
            arg = arg.substr(0, arg.size()-1);
            if(!arg.compare("guest")){
                _actualPwd = _pwd + "guest/";
                _user = arg;
                asio::async_write(_socket,asio::buffer("230 You may use server as a guest\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
            } else{
                _user = arg;
                asio::async_write(_socket,asio::buffer("331 Password required for "+arg+"\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
            }

        } else if(line.rfind("PASS", 0) == 0){
            std::string arg = line.substr(line.find(" ") + 1);
            arg = arg.substr(0, arg.size()-1);
            std::map<std::string,std::string>::iterator it = _users.find(_user);
            if (it != _users.end()){
                std::string correctPassword = _users.at(_user);
                if(correctPassword.compare(arg)){
                    _user = "";
                    _logger.log("INFO","User " + _user + " login failed.");
                    asio::async_write(_socket,asio::buffer("430 Wrong user of password password\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
                } else{
                    _actualPwd = _pwd + _user + "/";
                    _logger.log("INFO","User " + _user + " login successful.");
                    asio::async_write(_socket,asio::buffer("230 User logged in\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
                }
            } else{
                _user = "";
                _logger.log("INFO","User " + _user + " login failed.");
                asio::async_write(_socket,asio::buffer("430 Wrong user of password password\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
            }

        } else if(line.rfind("SYST", 0) == 0){
            asio::async_write(_socket,asio::buffer("215 UNKNOWN\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));

        } else if(line.rfind("PORT", 0) == 0){
            std::string arg = line.substr(line.find(" ") + 1);
            std::vector<std::string> data = split(arg,',');
            _connectionAddress = data[0]+"."+data[1]+"."+data[2]+"."+data[3];
            _connectionPort = std::stoi(data[4])*256+std::stoi(data[5]);
            asio::ip::tcp::endpoint ep(asio::ip::address::from_string(_connectionAddress), _connectionPort);
            asio::ip::tcp::socket* s = new asio::ip::tcp::socket(_context, ep.protocol());
            s->connect(ep);
            asio::async_write(_socket,asio::buffer("200 PORT Command successful\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));

            _connSocket.reset(s);

        } else if(line.rfind("LIST", 0) == 0){

            if(_connSocket!=nullptr){
                std::string arg = line.substr(line.find(" ") + 1);
                arg = arg.substr(0, arg.size()-1);
                if(!arg.compare("LIST") || arg.length() < 1){
                    asio::async_write(_socket,asio::buffer("125 Data connection already opened, transfer starting\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
                    std::string response;
                    for (const auto & entry : fsys::directory_iterator(_actualPwd)){
                        //std::cout << _pwd << " " << entry.path().string() << std::endl;
                        if(canAccessFile(entry.path().string(),_user,_pwd)){
                            if(fsys::is_directory(entry)){
                                response += "<DIR>  ";
                            } else{
                                response += "       ";
                            }
                            response += entry.path().string().substr(_actualPwd.length());
                            response += "\r\n";
                        }

                    }
                    asio::async_write(*_connSocket,asio::buffer(response+"\r\n"),std::bind(&FtpClient::_onTransmissionFinished,this,_1,_2));
                } else if(fsys::exists(_actualPwd+arg) && canEnterDirectory(_actualPwd+arg,_user,_pwd)){
                    asio::async_write(_socket,asio::buffer("125 Data connection already opened, transfer starting\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
                    std::string response;
                    for (const auto & entry : fsys::directory_iterator(_actualPwd+arg)){
                        if(canAccessFile(entry.path().string(),_user,_pwd)){
                            if(fsys::is_directory(entry)){
                                response += "<DIR>  ";
                            } else{
                                response += "       ";
                            }
                            response += entry.path().string().substr(_actualPwd.length());
                            response += "\r\n";
                        }

                    }
                    asio::async_write(*_connSocket,asio::buffer(response+"\r\n"),std::bind(&FtpClient::_onTransmissionFinished,this,_1,_2));
                } else{
                    asio::async_write(_socket,asio::buffer("550 The directory does not exist or you don't have permission to view it.\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
                }
            } else{
                asio::async_write(_socket,asio::buffer("425 Can't open data connection.\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
            }
        }
        else if(line.rfind("CWD", 0) == 0){
            std::string arg = line.substr(line.find(" ") + 1);
            arg = arg.substr(0, arg.size()-1);

            if(arg.rfind("~", 0) == 0 || arg.length() < 1){
                _actualPwd = _pwd+_user+"/";
                asio::async_write(_socket,asio::buffer("200 Directory changed.\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
            } else{
                if(canEnterDirectory(_actualPwd+arg+"/",_user,_pwd)){
                    _actualPwd = _actualPwd + arg + "/";
                    asio::async_write(_socket,asio::buffer("200 Directory changed.\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
                } else{
                    asio::async_write(_socket,asio::buffer("550 Requested action not taken. You cannot enter this directory or this directory does not exist.\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
                }
            }
        } else if(line.rfind("NOOP", 0) == 0){
            asio::async_write(_socket,asio::buffer("200 OK\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
        } else if(line.rfind("MKD", 0) == 0){
            std::string arg = line.substr(line.find(" ") + 1);
            arg = arg.substr(0, arg.size()-1);

            if(canWriteToDir(fsys::path(_actualPwd+arg).parent_path().string(),_user,_pwd)){
                fsys::create_directory(_actualPwd+arg);
                asio::async_write(_socket,asio::buffer("200 Directory created.\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
            } else{
                asio::async_write(_socket,asio::buffer("550 Requested action not taken. You are not authorized to do this action or the request is malformed.\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
            }

        } else if(line.rfind("RMD", 0) == 0){
            std::string arg = line.substr(line.find(" ") + 1);
            arg = arg.substr(0, arg.size()-1);

            if(canRemove(_actualPwd+arg,_user,_pwd)){
                fsys::remove_all(_actualPwd+arg);
                asio::async_write(_socket,asio::buffer("200 Directory removed.\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
            } else{
                asio::async_write(_socket,asio::buffer("550 Requested action not taken. You are not authorized to do this action or the request is malformed.\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
            }

        } else if(line.rfind("DELE", 0) == 0){
            std::string arg = line.substr(line.find(" ") + 1);
            arg = arg.substr(0, arg.size()-1);

            if(canRemove(_actualPwd+arg,_user,_pwd)){
                fsys::remove(_actualPwd+arg);
                asio::async_write(_socket,asio::buffer("200 File removed.\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
            } else{
                asio::async_write(_socket,asio::buffer("550 Requested action not taken. You are not authorized to do this action or the request is malformed.\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
            }

        }
         else if(line.rfind("RETR", 0) == 0){
            if(_connSocket!=nullptr){

                std::string response;
                std::string arg = line.substr(line.find(" ") + 1);
                arg = arg.substr(0, arg.size()-1);
                _logger.log("FILE","File transfer initialized by " + _user + "; file: "+_actualPwd+arg);
                if(fsys::exists(fsys::status(_actualPwd+arg))){
                    if(canAccessFile(_actualPwd+arg,_user,_pwd)){
                        if(!fsys::is_directory(fsys::status(_actualPwd+arg))){
                            std::ifstream t(_actualPwd+arg);
                            std::stringstream buffer;
                            buffer << t.rdbuf();
                            
                            asio::async_write(_socket,asio::buffer("125 Data connection already opened, transfer starting\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
                            asio::async_write(*_connSocket,asio::buffer(buffer.str()),std::bind(&FtpClient::_onTransmissionFinished,this,_1,_2));
                        } else{
                            asio::async_write(_socket,asio::buffer("550 Requested action not taken. File unavailable (file is a directory).\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
                        }
                    } else{
                        asio::async_write(_socket,asio::buffer("550 Requested action not taken. File unavailable (file not found or permission denied).\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
                    }
                } else {
                    asio::async_write(_socket,asio::buffer("550 Requested action not taken. File unavailable (file not found or permission denied).\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
                }
            } else{
                asio::async_write(_socket,asio::buffer("425 Can't open data connection.\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
            } 
        } else if(line.rfind("STOR", 0) == 0){
            if(_connSocket!=nullptr){
                std::string arg = line.substr(line.find(" ") + 1);
                arg = arg.substr(0, arg.size()-1);
                _logger.log("FILE","File transfer initialized by " + _user + "; file: "+_actualPwd+arg);
                if(canWriteToDir(_actualPwd,_user,_pwd)){
                    asio::async_write(_socket,asio::buffer("125 Data connection already opened, waiting for transfer\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
                    asio::async_read(*_connSocket,_dataBuff,std::bind(&FtpClient::_onReadingFinished,this,_1,_2));
                    _lastWrittenFile = _actualPwd+arg;
                } else{
                    asio::async_write(_socket,asio::buffer("550 Requested action not taken. Directory unavailable (directory not found or permission denied).\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
                }
            } else{
                asio::async_write(_socket,asio::buffer("425 Can't open data connection.\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
            }
        } else if(line.rfind("TYPE", 0) == 0){
            asio::async_write(_socket,asio::buffer("202 This server only accepts ASCII type (A). Command was ignored.\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
        } else if(line.rfind("MODE", 0) == 0){
            asio::async_write(_socket,asio::buffer("202 This server only accepts Stream mode (S). Command was ignored.\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
        } else if(line.rfind("STRU", 0) == 0){
            asio::async_write(_socket,asio::buffer("202 This server only accepts File Structure (F). Command was ignored.\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
        } else if(line.rfind("QUIT", 0) == 0){
            _socket.close();
            if(_connSocket!=nullptr) _connSocket->close();
            std::unique_ptr<FtpClient> moved(std::move(_clientsVector.at(_myId)));
            _clientsVector.erase(_clientsVector.begin()+_myId);
            _logger.log("CONN","Connection closing");
            return;
        } else{
            asio::async_write(_socket,asio::buffer("502 Command not implemented\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
        }
    }
    asio::async_read_until(_socket,_buff,"\r\n",std::bind(&FtpClient::_onread,this,_1,_2));
}

void FtpClient::connectMessage(){
    asio::async_write(_socket,asio::buffer(std::string("220 Service ready for new user")+"\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
}

void FtpClient::_onwrite(const asio::error_code& e, std::size_t size){

}

void FtpClient::_onTransmissionFinished(const asio::error_code& e, std::size_t size){
    asio::async_write(_socket,asio::buffer(std::string("226 Closing data connection. Requested action successful.")+"\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
    _connSocket->close();
}

void FtpClient::_onReadingFinished(const asio::error_code& e, std::size_t size){
    std::cout<< "reading finished" << std::endl;
    std::ofstream out(_lastWrittenFile);
    std::istream is(&_dataBuff);
    std::string s(std::istreambuf_iterator<char>(is), {});
    out << s;
    asio::async_write(_socket,asio::buffer(std::string("200 Closing data connection. Requested action successful.")+"\r\n"),std::bind(&FtpClient::_onread,this,_1,_2));
    _connSocket->close();
}

void FtpClient::setPWD(std::string pwd){
    _pwd = pwd;
    _actualPwd = pwd;
}


bool FtpClient::canEnterDirectory(std::string dir, std::string user, std::string drive){

    const auto dirAct = fsys::canonical(dir);
    const auto dirDrive = fsys::canonical(drive);
    const auto dirDriveUser = fsys::canonical(drive+user);
    const auto dirDriveGuest = fsys::canonical(drive+"guest");

    std::string pstring("public");

    if(!dirAct.compare(dirDrive)){
        return true;
    }
    if(isSafePath(dirDriveUser,dirAct)){
        return true;
    }
    if(isSafePath(dirDriveGuest,dirAct)){
        return true;
    }
    for(std::map<std::string,std::string>::iterator iter = _users.begin(); iter != _users.end(); ++iter){
        std::string u =  iter->first;
        const auto dirDriveOtherUser = fsys::canonical(drive+u);

        if(!dirDriveOtherUser.compare(dirAct)){
            return true;
        }
        if(fsys::exists(drive+u+"/public")){
            const auto dirDriveOtherUserPublic = fsys::canonical(drive+u+"/public");
            if(isSafePath(dirDriveOtherUserPublic,dirAct)){
                return true;
            }
        }
    }
    return false;
}

bool FtpClient::canAccessFile(std::string file, std::string user, std::string drive){

    const auto dirAct = fsys::canonical(file);
    const auto dirDrive = fsys::canonical(drive);
    const auto dirDriveUser = fsys::canonical(drive+user);
    const auto dirDriveGuest = fsys::canonical(drive+"guest");

    if(isSafePath(dirDriveUser,dirAct)){
        return true;
    }
    if(isSafePath(dirDriveGuest,dirAct)){
        return true;
    }
    for(std::map<std::string,std::string>::iterator iter = _users.begin(); iter != _users.end(); ++iter){
        std::string u =  iter->first;
        const auto dirDriveOtherUser = fsys::canonical(drive+u);

        if(!dirDriveOtherUser.compare(dirAct)){
            return true;
        }
        if(fsys::exists(drive+u+"/public")){
            const auto dirDriveOtherUserPublic = fsys::canonical(drive+u+"/public");
            if(isSafePath(dirDriveOtherUserPublic,dirAct)){
                return true;
            }
        }
    }

    return false;
}

bool FtpClient::canWriteToDir(std::string dir, std::string user, std::string drive){

    const auto dirAct = fsys::canonical(dir);
    const auto dirDrive = fsys::canonical(drive);
    const auto dirDriveUser = fsys::canonical(drive+user);
    const auto dirDriveGuest = fsys::canonical(drive+"guest");

    if(isSafePath(dirDriveUser,dirAct)){
        return true;
    }
    if(isSafePath(dirDriveGuest,dirAct)){
        return true;
    }
    for(std::map<std::string,std::string>::iterator iter = _users.begin(); iter != _users.end(); ++iter){
        std::string u =  iter->first;
        const auto dirDriveOtherUser = fsys::canonical(drive+u);
        if(fsys::exists(drive+u+"/public")){
            const auto dirDriveOtherUserPublic = fsys::canonical(drive+u+"/public");
            if(isSafePath(dirDriveOtherUserPublic,dirAct)){
                return true;
            }
        }
    }

    return false;
}

bool FtpClient::canRemove(std::string dir, std::string user, std::string drive){

    const auto dirAct = fsys::canonical(dir);
    const auto dirDrive = fsys::canonical(drive);
    const auto dirDriveUser = fsys::canonical(drive+user);
    const auto dirDriveGuest = fsys::canonical(drive+"guest");

    if(!fsys::exists(dirAct)){
        return false;
    }
    if(isSafePath(dirDriveUser,dirAct)){
        return true;
    }
    if(!dirDriveGuest.compare(dirAct)){
        return false;
    }
    if(isSafePath(dirDriveGuest,dirAct)){
        return true;
    }
    for(std::map<std::string,std::string>::iterator iter = _users.begin(); iter != _users.end(); ++iter){
        std::string u =  iter->first;
        const auto dirDriveOtherUser = fsys::canonical(drive+u);

        if(fsys::exists(drive+u+"/public")){
            const auto dirDriveOtherUserPublic = fsys::canonical(drive+u+"/public");
            if(!dirDriveOtherUserPublic.compare(dirAct)){
                return false;
            }
            if(isSafePath(dirDriveOtherUserPublic,dirAct)){
                return true;
            }
        }
    }

    return false;
}

void FtpClient::setId(int id){
    _myId = id;
}