#include"FtpServer.hpp"
using namespace std::placeholders;

FtpServer::FtpServer(std::string address, int port): _address(address), _port(port),
    _endpoint(asio::ip::make_address_v4(_address), _port), _acceptor(_context,_endpoint),
    _clientCounter(0) {

}

void FtpServer::run(){
    _logger.start();
    _acceptor.async_accept(std::bind(&FtpServer::_clientHandler,this,_1,_2));
    _context.run();
}

void FtpServer::_clientHandler(const asio::error_code& error, asio::ip::tcp::socket socket){
    std::unique_ptr<FtpClient> tc (new FtpClient(_clientCounter++,std::move(socket),_context,_users,_logger,_clients));
    tc->connectMessage();
    tc->setPWD(_drive);
    _clients.push_back(std::move(tc));
    _clients.back()->setId(_clients.size()-1);
    _acceptor.async_accept(std::bind(&FtpServer::_clientHandler,this,_1,_2));
    _logger.log("CONN","New client connected.");
}

void FtpServer::config(std::string filename){
    std::ifstream infile(filename);
    
    std::string line;
    while (std::getline(infile, line)){
        std::istringstream readstream(line);
        std::string key;
        std::string arg;

        std::getline(readstream, key, '=');
        std::getline(readstream, arg);

        if(!key.compare("drive")){
            _drive = arg+"/";
            std::cout << "setting drive to " << _drive << std::endl;
        }

        if(!key.compare("logfile")){
            _logger.setOutput(arg);
            std::cout << "setting logfile to " << arg << std::endl;
        }

    }
}

void FtpServer::loadUsers(std::string filename){
    std::ifstream infile(filename);
    
    std::string line;
    while (std::getline(infile, line)){
        std::istringstream readstream(line);
        std::string user;
        std::string password;

        std::getline(readstream, user, ',');
        std::getline(readstream, password);
        _users.insert({user,password});

    }
}