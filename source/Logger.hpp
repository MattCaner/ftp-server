#pragma once
#include"includes.hpp"

/**
 * @brief A simple logger that uses one output file and logs time together with two additional string values.
 */
class Logger{
public:
    /**
     * @brief Sets output to the given file.
     * 
     * @param filename Name of the output file, relative to server file location.
     */
    void setOutput(std::string filename);

    /**
     * @brief Initializes and starts the logger. Erases log file and stores information about logging start there.
     */
    void start();

    /**
     * @brief Logs the information in the output file as [date][type]what
     * 
     * @param type An information that will inform about the type or additional information of entry
     * @param what The value of the entry
     */
    void log(std::string type, std::string what);
    
private:
    std::ofstream _file;
};