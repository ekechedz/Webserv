#include "../include/Server.hpp"
#include "../include/Socket.hpp"
#include "../include/Request.hpp"
#include "../include/Webserver.hpp"
#include "../include/CGIHandler.hpp"
#include "../include/Logger.hpp"
#include "../include/Utils.hpp"
#include <dirent.h>
#include <algorithm>

Server::Server(const std::vector<ServerConfig>& configs)
	: _configs(configs)
{
	logInfo("Initializing server with " + intToStr(configs.size()) + " configurations");
	for (size_t i = 0; i < configs.size(); ++i)
	{
		const ServerConfig& config = configs[i];
		// Only creating a socket if the current server config is the first of that ip-port-combo
		if (findServerConfig(config.getHost(), config.getPort()) != &_configs[i])
			return;
		int sock = createListeningSocket(config);
		struct pollfd pfd;
		pfd.fd = sock;
		pfd.events = POLLIN;
		pfd.revents = 0;
		_pollFds.push_back(pfd);
		_sockets[sock] = Socket(sock, Socket::LISTENING, Socket::RECEIVING, config.getHost() , config.getPort());
		logInfo("Listening on " + config.getHost() + ":" + intToStr(config.getPort()));
	}
}

int Server::createListeningSocket(const ServerConfig &config)
{
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		logError("Socket creation failed: " + std::string(std::strerror(errno)));
		throw std::runtime_error("Socket creation failed");
	}

	int opt = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
	{
		std::string error = "Setsockopt failed: " + std::string(std::strerror(errno));
		logError(error);
		throw std::runtime_error(error);
	}
	sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(config.getPort());
	addr.sin_addr.s_addr = inet_addr(config.getHost().c_str());

	if (bind(sock, (sockaddr *)&addr, sizeof(addr)) == -1)
	{
		logError("Bind failed for " + config.getHost() + ":" + intToStr(config.getPort()) + " - " + std::string(std::strerror(errno)));
		throw std::runtime_error("Bind failed");
	}

	if (listen(sock, SOMAXCONN) == -1)
	{
		logError("Listen failed: " + std::string(std::strerror(errno)));
		throw std::runtime_error("Listen failed");
	}

	fcntl(sock, F_SETFL, O_NONBLOCK);
	return sock;
}

void Server::acceptConnection(Socket &listeningSocket)
{
		size_t currentClients = 0;
	for (std::map<int, Socket>::iterator it = _sockets.begin(); it != _sockets.end(); ++it)
	{
		if (it->second.getType() == Socket::CLIENT)
			++currentClients;
	}

	if (currentClients >= MAX_SOCKETS)
	{
		std::cerr << "Connection refused: MAX_CLIENTS reached.\n";

		sockaddr_in clientAddr;
		socklen_t len = sizeof(clientAddr);
		int clientFd = accept(listeningSocket.getFd(), (sockaddr *)&clientAddr, &len);
		if (clientFd != -1)
		{
			Response res;
			res.setStatus(503);
			res.setHeader("Connection", "close");
			res.setHeader("Content-Type", "text/html");

			std::string body =
				"<html><body><h1>503 Service Unavailable</h1><p>Server is too busy.</p></body></html>";
			res.setBody(body);
			res.setHeader("Content-Length", intToStr(body.size()));

			logWarning("Server too busy, rejecting new connection");
			std::string responseStr = res.toString();
			send(clientFd, responseStr.c_str(), responseStr.size(), 0);
			close(clientFd);
		}
		return;
	}
	sockaddr_in clientAddr;
	socklen_t len = sizeof(clientAddr);
	int clientFd = accept(listeningSocket.getFd(), (sockaddr *)&clientAddr, &len);
	if (clientFd == -1)
	{
		logError("Failed to accept new connection: " + std::string(std::strerror(errno)));
		return;
	}

	fcntl(clientFd, F_SETFL, O_NONBLOCK);

	// Adding client fd to Server::_pollFds vector
	pollfd pfd = {clientFd, POLLIN, 0};
	_pollFds.push_back(pfd);

	_sockets[clientFd] = Socket(clientFd, Socket::CLIENT, Socket::RECEIVING, listeningSocket.getIPv4(), listeningSocket.getPort());
	logInfo("Accepted new connection on fd " + intToStr(clientFd));
}

void Server::handleClientTimeouts()
{
	for (std::map<int, Socket>::iterator it = _sockets.begin(); it != _sockets.end();)
	{
		Socket &client = it->second;
		if (client.getType() != Socket::LISTENING && time(NULL) - client.getLastActivity() > 30)
		{
			logInfo("Client " + intToStr(client.getFd()) + " has timed out. Closing connection.");
			close(client.getFd());
			_sockets.erase(it++);
		}
		else
			++it;
	}
}

void Server::run()
{
	logInfo("Server started and ready to accept connections");
	while (true)
	{
		int ret = poll(_pollFds.data(), _pollFds.size(), 5000);
		if (ret == -1)
		{
			logError("Poll error occurred");
			std::cerr << "Poll error\n";
			break;
		}
		else if (ret == 0)
		{
			handleClientTimeouts(); // could be testet with telnet
			continue;
		}
		for (size_t i = 0; i < _pollFds.size(); ++i)
		{
			// if (_pollFds[i].revents & POLLIN)
			if (_pollFds[i].events & _pollFds[i].revents)
			{
				int fd = _pollFds[i].fd;

				// Check if socket still exists (could be deleted during previous iteration)
				std::map<int, Socket>::iterator it = _sockets.find(fd);
				if (it == _sockets.end())
					continue;

				if (it->second.getType() == Socket::LISTENING)
					acceptConnection(it->second);
				else
				{
					if (it->second.getState() == Socket::RECEIVING)
						handleClient(it->second);
					else // Socket::SENDING
						sendResponse(it->second);
				}
			}
		}
	}
}

void Server::printSockets()
{
	logDebug("===== Socket List =====");
	std::ostringstream socketInfo;
	for (std::map<int, Socket>::iterator it = _sockets.begin(); it != _sockets.end(); ++it)
	{
		socketInfo << "Key in map: " << it->first << ", " << it->second;
	}
	logDebug(socketInfo.str());
	std::cout << socketInfo.str();

	socketInfo.str("");
	socketInfo << "===== Poll Fd List =====";
	for (std::vector<pollfd>::iterator it = _pollFds.begin(); it != _pollFds.end(); ++it)
	{
		socketInfo << "\nFd: " << it->fd
				  << ", Event: " << decodeEvents(it->events)
				  << ", Revent: " << decodeEvents(it->revents);
	}
	logDebug(socketInfo.str());
	std::cout << socketInfo.str() << std::endl;
}

// Prepares the buffer that will be sent to the client
// Historically, this function sent the response to the client, but now it only prepares the buffer
void Server::makeReadyforSend(Response& response, Socket& client)
{
	// Converting the response class to a string and storing it in the client's buffer
	std::string string = response.toString();
	client.clearBuffer();
	client.appendToBuffer(string.c_str(), string.size());

	// Setting the client state to SENDING
	client.setState(Socket::SENDING);

	// Changing the pollfd event to POLLOUT
	pollfd& pfd = findPollFd(client.getFd());
	pfd.events = POLLOUT;
	pfd.revents = 0;

	// Preparing connection close if needed
	if (response.getHeaderValue("Connection") == "close")
		client.setNeedsToClose(true);
}

// Sends the response inside the socket's buffer to the client
void Server::sendResponse(Socket& client)
{
	// Sending the response to the client
	const std::string& buffer = client.getBuffer();
	ssize_t bufferSize = buffer.size();
	ssize_t bytesSent = send(client.getFd(), buffer.c_str(), bufferSize, 0);
	logDebug("Sent " + intToStr(bytesSent) + " bytes to client " + intToStr(client.getFd()));

	// If send failed, delete the client.
	// (Even though this is not expected, it should not terminate the server, so we don't throw an exception here.)
	if (bytesSent == -1)
	{
		logError("Send failed to client " + intToStr(client.getFd()) + ": " + std::string(strerror(errno)) + ", deleting client");
		deleteClient(client);
		return;
	}

	// Trimming the part of the buffer that was sent
	client.trimBuffer(bytesSent);
	logDebug("Trimmed buffer for client " + intToStr(client.getFd()) + ", new size: " + intToStr(client.getBuffer().size()));

	// If the buffer was not sent completely, return so that the rest of the response can be sent again later
	if (bytesSent < bufferSize)
	{
		logDebug("Sent partial response to client " + intToStr(client.getFd()) + ", bytes sent: " + intToStr(bytesSent));
		return;
	}
	logDebug("Sent full response to client " + intToStr(client.getFd()));

	// If the client needs to close the connection, delete it
	if (client.getNeedsToClose())
	{
		logInfo("Closing connection for and deleting client " + intToStr(client.getFd()));
		deleteClient(client);
		return;
	}

	// Resetting the client's buffer and preparing it to receive data again
	client.clearBuffer();
	client.setState(Socket::RECEIVING);
	pollfd& pfd = findPollFd(client.getFd());
	pfd.events = POLLIN;
	pfd.revents = 0;
}

// Returns the first serverConfig from the list that matches IP and port
ServerConfig* Server::findServerConfig(const std::string IPv4, int port)
{
	for (size_t i = 0; i < _configs.size(); ++i)
	{
		if (_configs[i].getHost() == IPv4 && _configs[i].getPort() == port)
			return &_configs[i];
	}
	logWarning("No matching server configuration found for " + IPv4 + ":" + intToStr(port));
	return NULL;
}

// Returns the first serverConfig from the list that matches IP, port and server name
ServerConfig* Server::findExactServerConfig(const std::string IPv4, int port, std::string serverName)
{
	for (size_t i = 0; i < _configs.size(); ++i)
	{
		if (_configs[i].getHost() == IPv4 && _configs[i].getPort() == port && _configs[i].getServerName() == serverName)
			return &_configs[i];
	}
	return NULL;
}


pollfd& Server::findPollFd(int targetFD)
{
	for (size_t i = 0; i < _pollFds.size(); ++i)
	{
		if (_pollFds[i].fd == targetFD)
			return _pollFds[i];
	}
	throw std::runtime_error("PollFd not found for fd: " + intToStr(targetFD));
}
