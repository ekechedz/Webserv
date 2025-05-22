#pragma once

#include <string>
#include <iostream>
#include <fstream>

class Logger 
{
public:
    enum Level { DEBUG, INFO, WARNING, ERROR, CRITICAL };

    static Logger& getInstance();

    void setLevel(Level level);
    void setLogFile(const std::string& filename);
    void log(Level level, const std::string& message);

private:
    Logger();
    ~Logger();
    Logger(const Logger&);
    //Logger& operator=(const Logger&);

    Level currentLevel;
    std::ofstream logFile;
    bool fileOutputEnabled;

    std::string levelToString(Level level) const;
    std::string getTimestamp() const;
};
