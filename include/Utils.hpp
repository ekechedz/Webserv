#pragma once
#include <string>
#include <poll.h>

int printError(const std::string &msg, int exitCode = 1);
std::string removeSemicolon(const std::string &str);
std::string intToStr(int num);
std::string decodeChunkedBody(std::istream &stream);
std::string decodeEvents(short int events);
