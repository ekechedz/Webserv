#pragma once
#include <iostream>
#include <string>
#include <sstream>

int printError(const std::string &msg, int exitCode = 1);
std::string removeSemicolon(const std::string &str);
std::string getlineCarriage(std::istringstream& iss);
std::string trim(const std::string& str);
