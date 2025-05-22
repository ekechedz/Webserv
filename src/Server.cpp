#include "../include/Server.hpp"
#include "../include/Socket.hpp"
#include "../include/Request.hpp"
#include "../include/Webserver.hpp"
#include "../include/CGIHandler.hpp"
#include "../include/Logger.hpp"
#include "../include/Utils.hpp"

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

	// Fallback to "/" location if no match
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
}

void Server::handleClient(Socket &client)
{
	char buffer[10000];
	ssize_t bytes = recv(client.getFd(), buffer, sizeof(buffer) - 1, 0);
	if (bytes <= 0)
	{
		deleteClient(client);
		;
		return;
	}
	buffer[bytes] = '\0';
	client.appendToBuffer(buffer);
	std::string requestString = client.getBuffer();

	Request req;
	Response res;
	parseHttpRequest(requestString, req, res);
	if (res.getStatus() == 400)
	{
		res.setHeader("Connection", "close");
		sendResponse(res, client);
		return;
	}

	// Checking if the request contains a "Host" header and returning 'Bad Request' if not
	if (req.getHeaders().count("Host") == 0)
	{
		res.setStatus(400);
		res.setHeader("Connection", "close");
		sendResponse(res, client);
		return;
	}

	// Getting server config based on the "Host" header
	{
		// Getting the first serverConf that matches ip, port and name
		ServerConfig *serverConfig = findExactServerConfig(client.getIPv4(), client.getPort(), req.getHeader("Host"));
		// If name does not match: Getting the first serverConf that matches ip and port
		if (!serverConfig)
			serverConfig = findServerConfig(client.getIPv4(), client.getPort());
		// If still no match, something went wrong
		if (!serverConfig)
			throw std::runtime_error("Unexpected: ServerConfig not found.");
		req.setServerConfig(serverConfig);
	}


	client.increaseNbrRequests();

	std::map<std::string, std::string> headers = req.getHeaders();
	std::string connectionHeader;

	if (headers.count("Connection"))
		connectionHeader = headers["Connection"];
	else
		connectionHeader = (req.getProtocol() == "HTTP/1.1") ? "keep-alive" : "close";

	if (client.getNbrRequests() >= MAX_REQUESTS)
		connectionHeader = "close";


	res.setHeader("Connection", connectionHeader);

	if (req.getProtocol() != "HTTP/1.1")
	{
		res.setStatus(505);
		res.setHeader("Connection", "close");
		sendResponse(res, client);
		return;
	}


	// Getting location config based on server config
	const std::vector<LocationConfig> &locations = req.getServerConfig()->getLocations();
	matchLocation(req, locations);

	const LocationConfig *loc = req.getMatchedLocation();
	if (loc)
	{
		if (req.getPath() == "/")
			req.setPath(req.getServerConfig()->getIndex());
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

		// Refactored CGI handling
		if (handleCgiRequest(req, res, loc, client))
			return;
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
}

bool Server::handleCgiRequest(const Request& req, Response& res, const LocationConfig* loc, Socket& client) {
	if (!loc || loc->getCgiPath().empty() || loc->getCgiExt().empty())
		return false;
	std::string ext = req.getPath().substr(req.getPath().find_last_of("."));
	if (ext != loc->getCgiExt())
		return false;

	logInfo("Processing CGI request: " + req.getPath());
	CGIHandler cgi(req, *loc);
	std::string cgiOutput = cgi.run();
	if (cgi.wasSuccessful()) {
		logInfo("CGI execution successful: " + req.getPath());
		res.setStatus(200);
		res.parseCgiOutput(cgiOutput);
	} else {
		logError("CGI execution failed: " + cgi.getError());
		res.setStatus(500);
		res.setHeader("Content-Type", "text/plain");
		res.setBody("CGI execution failed: " + cgi.getError());
	}
	sendResponse(res, client);
	client.clearBuffer();
	return true;
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
void Server::deleteClient(Socket &client)
{
	logInfo("Closing connection with client " + intToStr(client.getFd()));
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

void Server::sendResponse(Response &response, Socket &client)
{
	std::string string = response.toString();
	ssize_t sent = send(client.getFd(), string.c_str(), string.size(), 0);
	if (sent == -1) // just to be sure that send does not fail
	{
		logError("Send failed to client " + intToStr(client.getFd()) + ": " + std::string(strerror(errno)));
		perror("send failed");
		deleteClient(client);
		;
		return;
	}

	if (response.getHeaderValue("Connection") == "close")
		deleteClient(client);
	;
}

void Server::handleGetRequest(Response &res, const std::string &path)
{
	std::string fullPath = "www" + path;
	std::ifstream file(fullPath.c_str(), std::ios::binary);

	if (!file.is_open())
	{
		logWarning("404 Not Found: " + fullPath);
		
		std::string body = "<html><body><h1>404 Not Found</h1></body></html>";
		res.setStatus(404);
		res.setHeader("Content-Type", "text/html");
		res.setHeader("Content-Length", intToStr(body.size()));
		res.setBody(body);
		return;
	}
	else
	{
		std::ostringstream content;
		content << file.rdbuf();

		std::string type = getContentType(fullPath);
		logInfo("200 OK: " + fullPath + " (" + type + ")");
		res.setStatus(200);
		res.setHeader("Content-Type", type);
		res.setHeader("Content-Length", intToStr(content.str().size()));
		res.setBody(content.str());
	}
}

void Server::handlePostRequest(Response &res, const std::string &path, const std::string &requestBody)
{
	// Construct full file path based on web root
	std::string fullPath = "www" + path;

	// Attempt to write the body directly to the file
	std::ofstream outFile(fullPath.c_str());
	if (!outFile.is_open())
	{
		logError("Failed to open file for writing: " + fullPath);
		res.setStatus(500);
		std::string err = "Failed to open file for writing: " + fullPath;
		res.setHeader("Content-Type", "text/plain");
		res.setHeader("Content-Length", intToStr(err.size()));
		res.setBody(err);
		return;
	}

	outFile << requestBody;
	outFile.close();
	
	logInfo("POST request successful: " + fullPath);

	// Prepare simple HTML response (or switch to plain text)
	std::string body =
		"<html><body>\n"
		"<h1>POST Received</h1>\n"
		"<br>\n"
		"<p>Path: " +
		path + "</p>\n"
			   "</body></html>";

	res.setStatus(200);
	res.setHeader("Content-Type", "text/html");
	res.setHeader("Content-Length", intToStr(body.size()));
	res.setBody(body);
}

void Server::handleDeleteRequest(Response &res, const std::string &path)
{
	std::string fullPath = "www" + path;
	std::ostringstream body;

	if (std::remove(fullPath.c_str()) == 0)
	{
		logInfo("File deleted successfully: " + fullPath);
		res.setStatus(200);
		body << "<html><body><h1>File Deleted</h1><p>Deleted: " << path << "</p></body></html>";
	}
	else
	{
		if (errno == ENOENT)
		{
			logWarning("404 Not Found for DELETE: " + fullPath);
			res.setStatus(404);
			body << "<html><body><h1>404 Not Found</h1><p>File not found: " << path << "</p></body></html>";
		}
		else if (errno == EACCES || errno == EPERM)
		{
			logError("403 Forbidden for DELETE: " + fullPath);
			res.setStatus(403);
			body << "Permission denied";
		}
		else
		{
			logError("500 Internal Server Error for DELETE: " + fullPath + " - " + std::string(strerror(errno)));
			res.setStatus(500);
			body << "<html><body><h1>500 Internal Server Error</h1><p>Error deleting: " << path << "</p></body></html>";
		}
	}

	res.setBody(body.str());
	res.setHeader("Content-Type", "text/html");
	res.setHeader("Content-Length", intToStr(body.str().size()));
}
// Returns the first serverConfig from the list that matches IP and port
ServerConfig* Server::findServerConfig(const std::string IPv4, int port)
{
	for (size_t i = 0; i < _configs.size(); ++i)
	{
		if (_configs[i].getHost() == IPv4 && _configs[i].getPort() == port)
			return &_configs[i];
	}
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
