#pragma once
#include "LocationConfig.hpp"
#include <map>
#include <string>
#include <vector>
#include <iostream>

class ServerConfig {
public:
	std::string host;
	int port;
	std::string server_name;
	std::string root;
	std::string index;
	size_t client_max_body_size;
	std::map<int, std::string> error_pages;
	std::vector<LocationConfig> locations;

	ServerConfig();
	void parseBlock(std::istream &stream);
	void print() const;
};

void print_servers(std::vector<ServerConfig>& servers);