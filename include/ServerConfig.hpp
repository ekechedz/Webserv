#pragma once
#include "LocationConfig.hpp"
#include <map>
#include <string>
#include <vector>
#include <iostream>

class ServerConfig {
private:
	std::string host;
	int port;
	std::string server_name;
	std::string root;
	std::string index;
	size_t client_max_body_size;
	std::map<int, std::string> error_pages;
	std::vector<LocationConfig> locations;
public:

	ServerConfig();
	void parseBlock(std::istream &stream);
	void print() const;

	const std::string& getHost() const;
	int getPort() const;
	const std::string& getServerName() const;
	const std::string& getRoot() const;
	const std::string& getIndex() const;
	size_t getClientMaxBodySize() const;
	const std::map<int, std::string>& getErrorPages() const;
	const std::vector<LocationConfig>& getLocations() const;
	void initialisedCheck() const;

};

void print_servers(std::vector<ServerConfig>& servers);
