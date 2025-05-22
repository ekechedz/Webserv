#include "../include/Logger.hpp"
#include <ctime>

Logger::Logger()
    : currentLevel(INFO), fileOutputEnabled(false)
{
}

Logger::~Logger()
{
    if (logFile.is_open())
        logFile.close();
}

Logger& Logger::getInstance()
{
    static Logger instance;
    return instance;
}

void Logger::setLevel(Level level)
{
    currentLevel = level;
}

void Logger::setLogFile(const std::string& filename)
{
    if (logFile.is_open())
        logFile.close();
    logFile.open(filename.c_str(), std::ios::app);
    fileOutputEnabled = logFile.is_open();
}

void Logger::log(Level level, const std::string& message)
{
    if (level < currentLevel)
        return;
    std::string output = "[" + getTimestamp() + "] " + levelToString(level) + ": " + message;
    if (level >= ERROR)
        std::cerr << output << std::endl;
    else
        std::cout << output << std::endl;
    if (fileOutputEnabled && logFile.is_open())
    {
        logFile << output << std::endl;
        logFile.flush();
    }
}

std::string Logger::levelToString(Level level) const
{
    switch (level)
    {
        case DEBUG: return "DEBUG";
        case INFO: return "INFO";
        case WARNING: return "WARNING";
        case ERROR: return "ERROR";
        case CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

std::string Logger::getTimestamp() const
{
    std::time_t now = std::time(0);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    return buf;
}
