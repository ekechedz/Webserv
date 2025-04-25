#include "../include/Utils.hpp"
#include <iostream>

int printError(const std::string &msg, int exitCode) {
	std::cerr << "\033[1;31m[ERROR] " << msg << "\033[0m" << std::endl;
	return exitCode;
}
