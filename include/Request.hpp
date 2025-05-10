#pragma once

#include <iostream>
#include <map>
#include <stdexcept>

#include "ServerConfig.hpp"
#include "Utils.hpp"

class Request
{
public:
	Request(const ServerConfig& setServerConfig);
	void parse(char* buffer);
	void print();

private:
	std::string method;
	std::string path;
	std::string protocol;
	std::map<std::string, std::string> headers;
	std::string body;
	const ServerConfig& serverConfig;
};


