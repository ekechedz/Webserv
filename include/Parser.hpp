#ifndef PARSER_HPP
#define PARSER_HPP

#include "Webserver.hpp"

class Server;

class Parser
{
private:
	std::vector<Server> servList;
	std::vector<std::string> servConf;
	size_t numServers;

	void readConfigFile(const std::string &filePath);
	void splitServerBlocks();
	void parseServerBlocks();

public:
	Parser();
	~Parser();
	
	int parseServerConfig(const std::string& filePath);
	std::vector<Server> getServers() const;
};

#endif
