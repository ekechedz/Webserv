#pragma once

#include "ServerConfig.hpp"
#include "Socket.hpp"
#include "Response.hpp"
#include "Utils.hpp"

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
	void deleteClient(int clientFD);

	private:
	std::vector<ServerConfig> _configs;
	std::map<int, Socket> _sockets;
	std::vector<pollfd> _pollFds;

	int createListeningSocket(const ServerConfig &config);
	void acceptConnection(int listenFd);
	void handleClient(int clientFd);
	void handleClientTimeouts();
	void handleGetRequest(Response& res, const std::string &path);
	void handlePostRequest(Response& res, const std::string &path, const std::string &requestBody);
	void handleDeleteRequest(Response& res, const std::string &path);
	void printSockets();
};

std::string getContentType(const std::string &path);
