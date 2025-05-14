#include "../include/Server.hpp"
#include "../include/Socket.hpp"
#include "../include/Request.hpp"

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
		_sockets[sock] = Socket(sock, Socket::LISTENING, Socket::RECEIVING, &config);
		std::cout << "Listening on " << config.getHost() << ":" << config.getPort() << "\n";
	}
}

int Server::createListeningSocket(const ServerConfig &config)
{
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
		throw std::runtime_error("Socket creation failed");

	int opt = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
	{
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

void Server::acceptConnection(Socket& listeningSocket)
{
	sockaddr_in clientAddr;
	socklen_t len = sizeof(clientAddr);
	int clientFd = accept(listeningSocket.getFd(), (sockaddr *)&clientAddr, &len);
	if (clientFd == -1)
		return;
	fcntl(clientFd, F_SETFL, O_NONBLOCK);

	// Adding client fd to Server::_pollFds vector
	pollfd pfd = { clientFd, POLLIN, 0 };
	_pollFds.push_back(pfd);

	// Adding client to Server::_sockets map
	_sockets[clientFd] = Socket(clientFd, Socket::CLIENT, Socket::RECEIVING, &listeningSocket.getServerConfig());

	// DEBUG
	printSockets();
}

void matchLocation(Request &req, const std::vector<LocationConfig> &locations)
{
	const LocationConfig *bestMatch = NULL;
	size_t longestMatch = 0;

	for (size_t i = 0; i < locations.size(); ++i)
	{
		const std::string &locPath = locations[i].getPath();

		if (req.path.compare(0, locPath.size(), locPath) == 0 &&
			(req.path.size() == locPath.size() || req.path[locPath.size()] == '/'))

		{
			if (locPath.size() > longestMatch)
			{
				bestMatch = &locations[i];
				longestMatch = locPath.size();
			}
		}
	}

	//Fallback to "/" location if no match
	if (!bestMatch)
	{
		for (size_t i = 0; i < locations.size(); ++i)
		{
			if (locations[i].getPath() == "/")
			{
				bestMatch = &locations[i];
				break;
			}
		}
	}

	req.matchedLocation = bestMatch;

	if (bestMatch)
	{
		std::cout << "Matched Location Path: " << bestMatch->getPath() << "\n";
		std::cout << "Matched Root: " << bestMatch->getRoot() << "\n";
		std::cout << "Index: " << bestMatch->getIndex() << "\n";
		std::cout << "Autoindex: " << (bestMatch->isAutoindex() ? "on" : "off") << "\n";
		std::cout << "Allowed Methods: ";
		const std::vector<std::string> &methods = bestMatch->getMethods();
		for (size_t j = 0; j < methods.size(); ++j)
			std::cout << methods[j] << " ";
		std::cout << "\n";
		if (!bestMatch->getRedirect().empty())
			std::cout << "Redirect: " << bestMatch->getRedirect() << "\n";
		if (!bestMatch->getCgiPath().empty())
			std::cout << "CGI Path: " << bestMatch->getCgiPath() << "\n";
		if (!bestMatch->getCgiExt().empty())
			std::cout << "CGI Extension: " << bestMatch->getCgiExt() << "\n";
	}
	else
		std::cout << "No matched location.\n";
}

void Server::handleClient(Socket& client)
{
	char buffer[10000];
	ssize_t bytes = recv(client.getFd(), buffer, sizeof(buffer) - 1, 0);
	if (bytes <= 0)
		deleteClient(client);;

	buffer[bytes] = '\0';
	client.appendToBuffer(buffer);
	std::string request = client.getBuffer();
	const std::vector<LocationConfig> &locations = client.getServerConfig().getLocations();

	Request req;
	Response res;
	try
	{
		req = parseHttpRequest(request);
		std::cout << "Requested path: " << req.path << "\n";
		matchLocation(req, locations);
		// req.print();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to parse request: " << e.what() << "\n";
		res.setStatus(400);
		res.setHeader("Connection", "close");
		sendResponse(res, client);
		return;
	}

	if (req.protocol != "HTTP/1.1")
	{
		res.setStatus(505);
		sendResponse(res, client);
	}

	if (req.matchedLocation)
	{
		if (req.path == "/")
			req.path = client.getServerConfig().getIndex();

		if (!req.matchedLocation->getRedirect().empty())
		{
			res.setStatus(301);
			res.setHeader("Location", req.matchedLocation->getRedirect());
			res.setHeader("Connection", "close");
			std::string html =
				"<html><head><title>301 Moved</title></head><body>"
				"<h1>301 Moved Permanently</h1>"
				"<p>Redirecting to <a href=\"" +
				req.matchedLocation->getRedirect() + "\">" +
				req.matchedLocation->getRedirect() + "</a></p></body></html>";

			res.setBody(html);
			std::ostringstream oss;
			oss << html.size();
			res.setHeader("Content-Length", oss.str());
			sendResponse(res, client);
			return;
		}
	}

	if (req.method == "GET")
		handleGetRequest(res, req.path);
	else if (req.method == "POST")
		handlePostRequest(res, req.path, req.body);
	else if (req.method == "DELETE")
		handleDeleteRequest(res, req.path);
	else
		res.setStatus(405);
	res.setHeader("Connection", "close");
	sendResponse(res, client);
	client.clearBuffer();
	printSockets();
}

void Server::handleClientTimeouts()
{
	for (std::map<int, Socket>::iterator it = _sockets.begin(); it != _sockets.end();)
	{
		Socket &client = it->second;
		if (client.getType() != Socket::LISTENING && time(NULL) - client.getLastActivity() > 30)
		{
			std::cout << "Client " << client.getFd() << " has timed out. Closing connection.\n";
			close(client.getFd());
			_sockets.erase(it++);
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
				if (_sockets[fd].getType() == Socket::LISTENING)
					acceptConnection(_sockets[fd]);
				else
					handleClient(_sockets[fd]);
			}
		}
	}
}

// after removing a client or sending a response the indices in _pollFds may no longer match the map in _sockets
void Server::deleteClient(Socket& client)
{
	close(client.getFd());
	for (size_t i = 0; i < _pollFds.size(); ++i)
	{
		if (_pollFds[i].fd == client.getFd())
		{
			_pollFds.erase(_pollFds.begin() + i);
			break;
		}
	}
	_sockets.erase(client.getFd());
}

void Server::printSockets()
{
	std::cout << "===== Socket List =====\n";
	for (std::map<int, Socket>::iterator it = _sockets.begin(); it != _sockets.end(); ++it)
	{
		std::cout << "Key in map: " << it->first
			<< ", " << it->second;
	}
	std::cout << "===== Socket List End =====\n";
	std::cout << "===== Poll Fd List =====\n";
	for (std::vector<pollfd>::iterator it = _pollFds.begin(); it != _pollFds.end(); ++it) {
		std::cout << "Fd: " << it->fd << ", Event: " << decodeEvents(it->events) << ", Revent: " << decodeEvents(it->revents) << "\n";
	}
	std::cout << "===== Poll Fd List End =====\n";
}

void Server::sendResponse(Response& response, Socket& client)
{
	std::string string = response.toString();
	ssize_t sent = send(client.getFd(), string.c_str(), string.size(), 0);
	if (sent == -1) // just to be sure that send does not fail
	{
		perror("send failed");
		deleteClient(client);;
		return;
	}

	if (response.getHeaderValue("Connection") == "close")
		deleteClient(client);;
}
