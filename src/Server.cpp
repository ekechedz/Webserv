#include "../include/Server.hpp"

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
		_listeningSockets[sock] = config;
		std::cout << "Listening on " << config.host << ":" << config.port << "\n";
	}
}

int Server::createListeningSocket(const ServerConfig &config)
{
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
		throw std::runtime_error("Socket creation failed");

	int opt = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(config.port);
	addr.sin_addr.s_addr = inet_addr(config.host.c_str());

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

	fcntl(clientFd, F_SETFL, O_NONBLOCK);

	struct pollfd pfd;
	pfd.fd = clientFd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	_pollFds.push_back(pfd);

	_clientBuffers[clientFd] = "";
}

void Server::handleClient(int clientFd, size_t index)
{
	char buffer[4096];
	ssize_t bytes = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
	if (bytes <= 0)
	{
		close(clientFd);
		_pollFds.erase(_pollFds.begin() + index);
		_clientBuffers.erase(clientFd);
		return;
	}

	buffer[bytes] = '\0';
	std::string request(buffer);
	std::istringstream requestStream(request);
	std::string method, path, protocol;
	requestStream >> method >> path >> protocol;
	if (path == "/")
		path = "/index.html";
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
		_clientBuffers.erase(clientFd);
	}
}

void Server::run()
{
	while (true)
	{
		int ret = poll(_pollFds.data(), _pollFds.size(), -1);
		if (ret == -1)
		{
			std::cerr << "Poll error\n";
			break;
		}

		for (size_t i = 0; i < _pollFds.size(); ++i)
		{
			if (_pollFds[i].revents & POLLIN)
			{
				int fd = _pollFds[i].fd;
				if (_listeningSockets.count(fd))
					acceptConnection(fd);
				else
					handleClient(fd, i--);
			}
		}
	}
}
