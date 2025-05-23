#pragma once
#include <string>
#include <poll.h>

// Error handling functions
int printError(const std::string &msg, int exitCode = 1);
void logError(const std::string &msg);
void logWarning(const std::string &msg);
void logInfo(const std::string &msg);
void logDebug(const std::string &msg);

// Utility functions
std::string removeSemicolon(const std::string &str);
std::string intToStr(int num);
std::string decodeChunkedBody(std::istream &stream);
std::string decodeEvents(short int events);

// Directory listing utility functions
bool isDirectory(const std::string &path);
