#include "../include/ConfigParser.hpp"
#include "../include/LocationConfig.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

ConfigParser::ConfigParser(const std::string &filename) {
	loadFile(filename);
}

void ConfigParser::loadFile(const std::string &filename) {
	std::ifstream file(filename.c_str());
	if (!file.is_open())
		throw std::runtime_error("Could not open config file.");

	std::stringstream ss;
	std::string line;
	while (std::getline(file, line)) {
		ss << cleanLine(line) << '\n';
	}
	_fileContent = ss.str();
	file.close();
}

std::string ConfigParser::cleanLine(const std::string &line) {
	std::string trimmed;
	for (size_t i = 0; i < line.size(); ++i) {
		if (line[i] == '#') break;
		trimmed += line[i];
	}
	return trimmed;
}

std::vector<ServerConfig> ConfigParser::parse() {
	std::vector<ServerConfig> servers;
	std::istringstream stream(_fileContent);
	std::string line;

	while (std::getline(stream, line)) {
		if (line.find("server") != std::string::npos) {
			ServerConfig server;
			server.parseBlock(stream);
			servers.push_back(server);
		}
	}
	return servers;
}
