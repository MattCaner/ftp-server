#include"Logger.hpp"

void Logger::setOutput(std::string filename){
    _file = std::ofstream(filename);
}

void Logger::start(){
    std::time_t now_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string time_formatted = std::ctime(&now_time);
    _file << "[" << time_formatted.substr(0,time_formatted.length()-1) << "]" << "[INFO] SERVER LOGS STARTED" << std::endl;
}

void Logger::log(std::string type,std::string what){
    std::time_t now_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string time_formatted = std::ctime(&now_time);
    _file << "[" << time_formatted.substr(0,time_formatted.length()-1) << "]" << "[" << type << "] " << what << std::endl;
}