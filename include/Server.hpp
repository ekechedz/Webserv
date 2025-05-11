#pragma once

#include "ServerConfig.hpp"
#include "Client.hpp"
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

struct SocketContext {
	enum Type { LISTENING, CLIENT } type;
	enum State { RECEIVING, SENDING } state;
	const ServerConfig* server;
};

class Server
{
public:
	Server(const std::vector<ServerConfig> &configs);
	void run();

private:
	std::vector<ServerConfig> _configs;
	std::vector<pollfd> _pollFds;
	std::map<int, SocketContext> _socketInfo;
	std::map<int, Client> _clients;

	int createListeningSocket(const ServerConfig &config);
	void acceptConnection(int listenFd);
	void handleClient(int clientFd, size_t index);
	void handleClientTimeouts();
	void handleGetRequest(int clientFd, const std::string &path);
	void handlePostRequest(int clientFd, const std::string &path, const std::string &requestBody);
	void handleDeleteRequest(int clientFd, const std::string &path);
};

std::string getContentType(const std::string &path);

