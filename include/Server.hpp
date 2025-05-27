#pragma once

#include "ServerConfig.hpp"
#include "Socket.hpp"
#include "Response.hpp"
#include "Utils.hpp"
#include "Request.hpp"
#include "LocationConfig.hpp"

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
	std::map<int, Socket> _sockets;
	std::vector<pollfd> _pollFds;

	int createListeningSocket(const ServerConfig &config);
	void acceptConnection(Socket& listeningSocket);
	void handleClient(Socket& client);
	void handleClientTimeouts();
	void handleGetRequest(Response& res, const Request& req);
	void handlePostRequest(Request &req, Response &res, const std::string &path, const std::string &requestBody);
	void handleDeleteRequest(Response& res, const std::string &path);
	bool handleCgiRequest(const Request& req, Response& res, const LocationConfig* loc, Socket& client);
	void list_directory(const std::string &path, Response& res);
	void printSockets();
	void makeReadyforSend(Response& response, Socket& client);
	void sendResponse(Socket& client);
	void deleteClient(Socket& client);
	ServerConfig* findServerConfig(const std::string IPv4, int port);
	ServerConfig* findExactServerConfig(const std::string IPv4, int port, std::string serverName);
	pollfd& findPollFd(int targetFD);
};

std::string getContentType(const std::string &path);
