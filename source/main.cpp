#include"FtpServer.hpp"

int main(){
    FtpServer server("127.0.0.1",21);
    server.config("server.config");
    server.loadUsers("users.txt");
    server.run();
    return 0;
}