#include "../include/Server.hpp"
#include "../include/Client.hpp"

Server::Server(const std::vector<ServerConfig> &configs)
	: _configs(configs)
{
	for (size_t i = 0; i < configs.size(); ++i)
	{
		const ServerConfig &config = configs[i];
		int sock = createListeningSocket(config);
		struct pollfd pfd;
		pfd.fd = sock;
		pfd.events = POLLIN;
		pfd.revents = 0;
		_pollFds.push_back(pfd);
		_socketInfo[sock].type = SocketContext::LISTENING;
		_socketInfo[sock].state = SocketContext::RECEIVING;
		_socketInfo[sock].server = &config;
		std::cout << "Listening on " << config.getHost() << ":" << config.getPort() << "\n";
	}
}

int Server::createListeningSocket(const ServerConfig &config)
{
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
		throw std::runtime_error("Socket creation failed");

	int opt = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
		std::ostringstream oss;
		oss << "Setsockopt failed: " << std::strerror(errno);
		throw std::runtime_error(oss.str());
	}
	sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(config.getPort());
	addr.sin_addr.s_addr = inet_addr(config.getHost().c_str());

	if (bind(sock, (sockaddr *)&addr, sizeof(addr)) == -1)
		throw std::runtime_error("Bind failed");

	if (listen(sock, SOMAXCONN) == -1)
		throw std::runtime_error("Listen failed");

	fcntl(sock, F_SETFL, O_NONBLOCK);
	return sock;
}

void Server::acceptConnection(int listenFd)
{
	sockaddr_in clientAddr;
	socklen_t len = sizeof(clientAddr);
	int clientFd = accept(listenFd, (sockaddr *)&clientAddr, &len);
	if (clientFd == -1)
		return;

	_clients[clientFd] = Client(clientFd);
	fcntl(clientFd, F_SETFL, O_NONBLOCK);

	struct pollfd pfd;
	pfd.fd = clientFd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	_pollFds.push_back(pfd);
	_socketInfo[clientFd].type = SocketContext::CLIENT;
	_socketInfo[clientFd].state = SocketContext::RECEIVING;
	_socketInfo[clientFd].server = _socketInfo[listenFd].server;
}

void Server::handleClient(int clientFd, size_t index)
{
	char buffer[4096];
	ssize_t bytes = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
	if (bytes <= 0)
	{
		close(clientFd);
		_pollFds.erase(_pollFds.begin() + index);
		_clients.erase(clientFd);
		return;
	}

	buffer[bytes] = '\0';
	std::string request(buffer); // original buffer to string
	_clients[clientFd].appendToBuffer(request);
	request = _clients[clientFd].getBuffer();
	std::istringstream requestStream(request);
	std::string method, path, protocol;
	requestStream >> method >> path >> protocol;
	if (path == "/")
		path = _socketInfo[clientFd].server->getIndex();
	if (method == "GET")
		handleGetRequest(clientFd, path);
	else if (method == "POST")
		handlePostRequest(clientFd, path, request);
	else if (method == "DELETE")
		handleDeleteRequest(clientFd, path);
	else
	{
		std::string response = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
		send(clientFd, response.c_str(), response.size(), 0);
		close(clientFd);
		_pollFds.erase(_pollFds.begin() + index);
		_clients.erase(clientFd);
	}
	_clients[clientFd].clearBuffer();
}

void Server::handleClientTimeouts()
{
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end();)
	{
		Client &client = it->second;
		if (time(NULL) - client.getLastActivity() > 30)
		{
			std::cout << "Client " << client.getFd() << " has timed out. Closing connection.\n";
			close(client.getFd());
			_clients.erase(it++);
		}
		else
			++it;
	}
}

void Server::run()
{
	while (true)
	{
		int ret = poll(_pollFds.data(), _pollFds.size(), 5000);
		if (ret == -1)
		{
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
			if (_pollFds[i].revents & POLLIN)
			{
				int fd = _pollFds[i].fd;
				if (_socketInfo[fd].type == SocketContext::LISTENING)
					acceptConnection(fd);
				else
					handleClient(fd, i);
			}
		}
	}
}
