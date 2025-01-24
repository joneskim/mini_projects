#include <iostream>
#include <fstream>
#include <string>

enum class LogLevel {
    INFO,
    WARNING,
    ERROR
};


class Logger {
    public:
        void log(logLevel level, const std::string& message){
            
        }

    private:

        // the constructor opens the file
        Logger(const std::string& logFile = "log.txt") : logFile(logFile) {
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
}