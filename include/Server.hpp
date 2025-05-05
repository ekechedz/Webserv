#pragma once

#include "ServerConfig.hpp"
#include <vector>
#include <map>
#include <poll.h>
#include <stdexcept>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <sstream>


class Server
{
public:
	Server(const std::vector<ServerConfig> &configs);
	void run();

private:
	std::vector<ServerConfig> _configs;
	std::vector<pollfd> _pollFds;
	std::map<int, ServerConfig> _listeningSockets;
	std::map<int, std::string> _clientBuffers;

	int createListeningSocket(const ServerConfig &config);
	void acceptConnection(int listenFd);
	void handleClient(int clientFd, size_t index);
	void handleGetRequest(int clientFd, const std::string &path);
	void handlePostRequest(int clientFd, const std::string &path, const std::string &requestBody);
	void handleDeleteRequest(int clientFd, const std::string &path);
};

std::string getContentType(const std::string &path);

