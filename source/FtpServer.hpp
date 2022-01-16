#pragma once
#include"includes.hpp"
#include"FtpClient.hpp"

/**
 * @brief A class that handles FTP clients and provides config.
 * 
 */
class FtpServer{
public:
    /**
     * @brief Construct a new Ftp Server object. It will yet have to be configurated and started.
     * 
     * @param address Address at which the server is to be started.
     * @param port Port at which the server is to be started (21, which is default, may require admin privileges).
     */
    FtpServer(std::string address, int port = 21);
    
    /**
     * @brief Load a config file. Config file is required to contain a name of log file and working directory that FTP will use as its drive.
     * 
     * @param filename Filename of config file, relative to server file location.
     */
    void config(std::string filename);
    
    /**
     * @brief Load users from a csv-like file that contains pairs user,password stored in each line. Each user is required to have their directory in drive directory provided in config.
     * 
     * @param filename Filename of users file, relative to server file location.
     */
    void loadUsers(std::string filename);
    
    /**
     * @brief Starts the server.
     * 
     */
    void run();

private:

    void _clientHandler(const asio::error_code& error, asio::ip::tcp::socket socket);

    std::string _address;
    int _port;
    asio::io_context _context;
    asio::ip::tcp::endpoint _endpoint;
    asio::ip::tcp::acceptor _acceptor;
    std::vector<std::unique_ptr<FtpClient>> _clients;
    int _clientCounter;
    std::string _drive;
    std::map<std::string, std::string> _users;
    Logger _logger;
};
