#include "../include/Server.hpp"
#include <cstring>
#include <fcntl.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <cerrno>
// here i am still just thinking about it but my mai idea is to set a socket
// which will listen for incoming conections

Server::Server(const ServerConfig &config)
	: _config(config) {
}

Server::~Server()
{
    // Clean up any resources
}
void Server::setupServerSocket()
{

	int opt = 1;
	_serverSocket = socket(AF_INET, SOCK_STREAM, 0); // Creating the socket, ipv4 tcp socket
	if (_serverSocket == -1)
		throw std::runtime_error("Failed to create socket");
	if(setsockopt(_serverSocket, SOL_SOCKET,SO_REUSEADDR, &opt, sizeof(opt)) == -1)
		throw std::runtime_error("Failed to set socket options");

	// creating a struct to hold the serv address
	struct sockaddr_in serverAddress;
	std::memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr(_config.host.c_str()); // seting the host
	serverAddress.sin_port = htons(_config.port);
	if (bind(_serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
		throw std::runtime_error("Failed to bind socket to address");
	if (listen(_serverSocket, SOMAXCONN) == -1) // max value of the pending conections
		throw std::runtime_error("Failed to listen on socket");
	struct pollfd serverPollFd;
    serverPollFd.fd = _serverSocket;
    serverPollFd.events = POLLIN;  // Monitoring for incoming connections
    _pollFds.push_back(serverPollFd);
}

// this function wait for the client and when it is find tries to connect to the server
void Server::acceptNewConnection()
{
	struct sockaddr_in clientAddress;
	socklen_t clientAddressLen= sizeof(clientAddress);
	int clientSock;
	int flags;

	clientSock = accept(_serverSocket, (struct sockaddr *)&clientAddress, &clientAddressLen);
	if(clientSock == -1)
	{
		std::cerr << "Error accepting new connection: " << strerror(errno) << std::endl;
		return;
	}

	flags = fcntl(clientSock, F_GETFL, 0);
	fcntl(clientSock, F_SETFL, flags | O_NONBLOCK); // setting the client to non-blocking mode
	struct pollfd newFd;
	newFd.fd = clientSock;
	newFd.events = POLLIN;
	newFd.revents = 0;

	_pollFds.push_back(newFd);
}

void Server::sendResponse(int clientSocket, const std::string &response) {
	ssize_t bytesSent = send(clientSocket, response.c_str(), response.length(), 0);  // Send the response.
	if (bytesSent == -1)
		std::cerr << "Error sending response: " << strerror(errno) << std::endl;
}

void Server::handleRequest(int clientSocket) {
	char buffer[1024];
	ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
	if (bytesRead <= 0)
	{
		close(clientSocket);
		return;
	}
	buffer[bytesRead] = '\0';
	std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nHello, World!";
	sendResponse(clientSocket, response);
}

void Server::run()
{
	setupServerSocket();
	// for easier debug
	std::cout << "Serv listen on " << _config.host << ":" << _config.port << std::endl;

	int pollResult;
	size_t i;
	while (true)
	{
		// here i choosed poll because from what i read select has a
		// limit on how much fd you can monitor and epoll its only for linux
		// for me the best choice is the poll.
		pollResult = poll(_pollFds.data(), _pollFds.size(), -1);
		if (pollResult == -1)
		{
			std::cerr << "Error with poll: " << strerror(errno) << std::endl;
			break;
		}
		// check the server for new connections
		if (_pollFds[0].revents & POLLIN)
			acceptNewConnection();
		for (i = 1; i <_pollFds.size(); ++i){
			if(_pollFds[i].revents & POLLIN)
				handleRequest(_pollFds[i].fd);
		}
	}
}
