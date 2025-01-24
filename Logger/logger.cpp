#include <iostream>
#include <fstream>
#include <string>

enum class logLevel {
    INFO,
    WARNING,
    ERROR,
};


class Logger {
    public:
        static Logger& getInstance(){
            static Logger instance("log.txt");
            return instance;
        }
        void log(logLevel lvl, const std::string& message){
            std::string levelStr;
            switch (lvl){
                case logLevel::INFO:
                    levelStr = "INFO";
                    break;
                case logLevel::WARNING:
                    levelStr = "WARNING";
                    break;
                case logLevel::ERROR:
                    levelStr = "ERROR";
                    break;
                default:
                    levelStr = "UNKNOWN";
                    break;
            }   

            file << levelStr << ": " << message << '\n';
        }

        // we can delete the copy constructor and assignment operator
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

    private:

        // the constructor opens the file
        Logger(const std::string& logFile) {
            file.open(logFile);
            if (!file.is_open()) {
                std::cerr << "Error opening log file" << std::endl;
            }
            file << "INFO: Logger started" << '\n';
        }
        // we can also use the destructor to close the file
        // with this we ensure the file is closed when the object is destroyed
        ~Logger() {
            if (file.is_open()) {
                file.close();
            }
        }

        std::string logFile;
        std::ofstream file;
};

int main() {
    Logger& logger = Logger::getInstance();
    logger.log(logLevel::INFO, "This is an informational message");
    logger.log(logLevel::WARNING, "This is a warning message");
    logger.log(logLevel::ERROR, "This is an error message");

    return 0;
}