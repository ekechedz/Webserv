#pragma once
#include "ServerConfig.hpp"
#include <string>
#include <vector>

class ConfigParser {
public:
	ConfigParser(const std::string &filename);
	std::vector<ServerConfig> parse();

private:
	std::string _fileContent;

	void loadFile(const std::string &filename);
	std::string cleanLine(const std::string &line);
};
