#pragma once
#include"Logger.hpp"
#include"includes.hpp"

/**
 * @brief An FTP client class, aimed at serving a single client connection. Most of the functionality is handled by its private methods.
 * 
 */
class FtpClient{
public:

    /**
     * @brief Construct a new Ftp Client object
     * 
     * @param myId Id of this object, should the server assign some special ids to its clients
     * @param socket ASIO socket that this client will take ownership of and that will use as a control commands socket
     * @param context ASIO server context
     * @param users Users map of the FTP server
     * @param logger Logger that this object will use to log its actions
     * @param clientsVector Vector of clients that this one client is a part of
     */
    FtpClient(int myId, asio::ip::tcp::socket&& socket, asio::io_context& context,std::map<std::string,std::string>& users, Logger& logger, std::vector<std::unique_ptr<FtpClient>>& clientsVector);
    
    /**
     * @brief Sets working directory (the general drive that the FTP server uses)
     * 
     * @param pwd FTP drive directory
     */
    void setPWD(std::string pwd);

    /**
     * @brief Sends a connect message to the client (220 Service ready for new user)
     * 
     */
    void connectMessage();

    /**
     * @brief Sets the Id object
     * 
     * @param id Id to set
     */
    void setId(int id);
    
private:


    void _onread(const asio::error_code& e, std::size_t size);
    void _onTransmissionFinished(const asio::error_code& e, std::size_t size);
    void _onwrite(const asio::error_code& e, std::size_t size);
    void _login(const asio::error_code& e, std::size_t size);
    void _onReadingFinished(const asio::error_code& e, std::size_t size);
    bool canEnterDirectory(std::string dir, std::string user, std::string drive);
    bool canAccessFile(std::string file, std::string user, std::string drive);
    bool canWriteToDir(std::string dir, std::string user, std::string drive);
    bool canRemove(std::string dir, std::string user, std::string drive);


    int _myId;
    asio::ip::tcp::socket _socket;
    asio::streambuf _buff;
    asio::streambuf _dataBuff;
    asio::io_context& _context;
    std::string _connectionAddress;
    std::string _user;
    int _connectionPort;
    std::unique_ptr<asio::ip::tcp::socket> _connSocket;
    std::unique_ptr<asio::streambuf> _connectionBuff;
    std::string _pwd;
    std::string _actualPwd;
    std::string _lastWrittenFile;
    std::map<std::string, std::string>& _users;
    Logger& _logger;
    std::vector<std::unique_ptr<FtpClient>>& _clientsVector;
};