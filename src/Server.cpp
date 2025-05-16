#include "../include/Server.hpp"
#include "../include/Socket.hpp"
#include "../include/Request.hpp"
#include "../include/Webserver.hpp"

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

		if (req.getPath().compare(0, locPath.size(), locPath) == 0 &&
			(req.getPath().size() == locPath.size() || req.getPath()[locPath.size()] == '/'))

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

	req.setMatchedLocation(bestMatch);

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
	{
		deleteClient(client);;
		return;
	}
	buffer[bytes] = '\0';
	client.appendToBuffer(buffer);
	std::string request = client.getBuffer();
	const std::vector<LocationConfig> &locations = client.getServerConfig().getLocations();

	Request req;
	Response res;
	try
	{
		req = parseHttpRequest(request);
		std::cout << "Requested path: " << req.getPath() << "\n";
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

	// Handle max nbr of requests per client
	client.increaseNbrRequests();
	if (client.getNbrRequests() == MAX_REQUESTS)
	{
		res.setHeader("Connection", "close");
	}

	// Checking protocol version and returning error if wrong
	if (req.getProtocol() != "HTTP/1.1")
	{
		res.setStatus(505);
		sendResponse(res, client);
		return;
	}

	// Closing connection if client requests it
	std::map<std::string, std::string> headers = req.getHeaders();
	if (headers.count("Connection") && headers["Connection"] == "close")
		res.setHeader("Connection", "close");

	const LocationConfig* loc = req.getMatchedLocation();
	if (loc)
	{
		if (req.getPath() == "/")
			req.setPath(client.getServerConfig().getIndex());

		if (!loc->getRedirect().empty())
		{
			res.setStatus(301);
			res.setHeader("Location", loc->getRedirect());

			std::string html =
				"<html><head><title>301 Moved</title></head><body>"
				"<h1>301 Moved Permanently</h1>"
				"<p>Redirecting to <a href=\"" +
				loc->getRedirect() + "\">" +
				loc->getRedirect() + "</a></p></body></html>";

			res.setBody(html);
			std::ostringstream oss;
			oss << html.size();
			res.setHeader("Content-Length", oss.str());
			sendResponse(res, client);
			return;
		}
	}

	std::string method = req.getMethod();
	std::string path = req.getPath();
	std::string body = req.getBody();

	if (method == "GET")
		handleGetRequest(res, path);
	else if (method == "POST")
		handlePostRequest(res, path, body);
	else if (method == "DELETE")
		handleDeleteRequest(res, path);
	else
		res.setStatus(405);


	sendResponse(res, client);
	client.clearBuffer();

	// DEBUG
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

void Server::handleGetRequest(Response& res, const std::string &path)
{
	std::string fullPath = "www" + path;
	std::ifstream file(fullPath.c_str(), std::ios::binary);

	// std::cout << "File opened successfully: " << fullPath << std::endl;

	if (!file.is_open())
	{
		res.setStatus(404);
		// std::cout << "File not found: " << fullPath << "\nStatus Code: 404 Not Found\n";
		return ;
	}
	else
	{
		std::ostringstream content;
		content << file.rdbuf();

		std::string type = getContentType(fullPath);
		res.setStatus(200);
		res.setHeader("Content-Type", type);
		res.setHeader("Content-Length", intToStr(content.str().size()));
		res.setBody(content.str());
	}
}

void Server::handlePostRequest(Response& res, const std::string &path, const std::string &requestBody)
{
	// std::cout << "Handling POST request to: " << path << std::endl;
	// std::cout << "Request body:\n" << requestBody << std::endl;

	std::map<std::string, std::string> formData;
	std::istringstream stream(requestBody);
	std::string pair;

	while (std::getline(stream, pair, '&'))
	{
		size_t eq = pair.find('=');
		if (eq != std::string::npos)
		{
			std::string key = pair.substr(0, eq);
			std::string value = pair.substr(eq + 1);
			formData[key] = value;
		}
	}

	// writing to a file
	std::ostringstream responseBody;
	responseBody << "<html>\n";
	responseBody << "<body>\n";
	responseBody << "<h1>POST Received</h1>\n";
	responseBody << "<br>\n";
	responseBody << "<p>Path: " << path << "</p>\n";
	responseBody << "<ul>\n";
	std::map<std::string, std::string>::iterator it;
	std::ofstream outFile("www/post_output.txt", std::ios::app);
	if (outFile.is_open())
	{
		for (it = formData.begin(); it != formData.end(); ++it)
			outFile << it->first << "=" << it->second << "\n";
		outFile << "----\n";
		outFile.close();
	}

	for (it = formData.begin(); it != formData.end(); ++it)
		responseBody << "<li>" << it->first << ": " << it->second << "</li>";
	responseBody << "</ul></body></html>";

	std::string body = responseBody.str();

	res.setStatus(200);
	res.setHeader("Content-Type", "text/html");
	res.setHeader("Content-Length", intToStr(body.size()));
	res.setBody(body);

	std::string response = res.toString();
}

void Server::handleDeleteRequest(Response& res, const std::string &path)
{
	std::string fullPath = "www" + path;
	std::ostringstream body;

	if (std::remove(fullPath.c_str()) == 0)
	{
		res.setStatus(200);
		body << "<html><body><h1>File Deleted</h1><p>Deleted: " << path << "</p></body></html>";
	}
	else
	{
		if (errno == ENOENT)
		{
			res.setStatus(404);
			body << "<html><body><h1>404 Not Found</h1><p>File not found: " << path << "</p></body></html>";
		}
		else if (errno == EACCES || errno == EPERM)
		{
			res.setStatus(403);
			body << "Permission denied";
		}
		else
		{
			res.setStatus(500);
			body << "<html><body><h1>500 Internal Server Error</h1><p>Error deleting: " << path << "</p></body></html>";
		}
	}

	res.setBody(body.str());
	res.setHeader("Content-Type", "text/html");
	res.setHeader("Content-Length", intToStr(body.str().size()));
}
