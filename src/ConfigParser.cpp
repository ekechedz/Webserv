#include "../include/ConfigParser.hpp"
#include "../include/LocationConfig.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <string>
#include <cstring>
#include <cstddef>
#include <string>

ConfigParser::ConfigParser(const std::string &filename)
{
	loadFile(filename);
}

void ConfigParser::loadFile(const std::string &filename)
{
	std::ifstream file(filename.c_str());
	if (!file.is_open())
		throw std::runtime_error("Could not open config file.");
	if (filename.rfind(".conf") != filename.length() - 5)
		throw std::invalid_argument("Invalid configuration file format");

	std::stringstream ss;
	std::string line;
	while (std::getline(file, line))
		ss << cleanLine(line) << '\n';
	_fileContent = ss.str();
	if (_fileContent.empty())
		throw std::runtime_error("Configuration file is empty.");
	file.close();
}

std::string ConfigParser::cleanLine(const std::string &line)
{
	std::string trimmed;
	for (size_t i = 0; i < line.size(); ++i)
	{
		if (line[i] == '#')
			break;
		trimmed += line[i];
	}
	return trimmed;
}
bool isPortUsed(const std::vector<ServerConfig>& servers, int port) {
	for (size_t i = 0; i < servers.size(); ++i) {
		if (servers[i].getPort() == port)
			return true;
	}
	return false;
}

std::vector<ServerConfig> ConfigParser::parse()
{
	std::vector<ServerConfig> servers;
	std::istringstream stream(_fileContent);
	std::string line;

	while (std::getline(stream, line))
	{
		if (line.find("server") != std::string::npos)
		{
			ServerConfig server;
			server.parseBlock(stream);
			server.initialisedCheck();
			if (isPortUsed(servers, server.getPort()))
				throw std::runtime_error("Matching port");
			servers.push_back(server);
		}
	}
	return servers;
}
