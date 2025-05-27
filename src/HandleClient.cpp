#include "../include/Server.hpp"
#include "../include/Socket.hpp"
#include "../include/Request.hpp"
#include "../include/Webserver.hpp"
#include "../include/CGIHandler.hpp"
#include "../include/Logger.hpp"
#include "../include/Utils.hpp"

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

void Server::handleClient(Socket &client)
{
	char buffer[30000];
	ssize_t bytes = recv(client.getFd(), buffer, sizeof(buffer) - 1, 0);
	if (bytes <= 0)
	{
		deleteClient(client);
		;
		return;
	}
	buffer[bytes] = '\0';
	client.appendToBuffer(buffer, bytes);
	std::string requestString = client.getBuffer();
	size_t headerEnd = requestString.find("\r\n\r\n");
	if (headerEnd != std::string::npos)
	{
		size_t contentLengthPos = requestString.find("Content-Length:");
		if (contentLengthPos != std::string::npos)
		{
			size_t lenStart = contentLengthPos + 15;
			while (lenStart < requestString.size() && (requestString[lenStart] == ' ' || requestString[lenStart] == '\t'))
				++lenStart;
			size_t lenEnd = requestString.find("\r\n", lenStart);
			int contentLength = atoi(requestString.substr(lenStart, lenEnd - lenStart).c_str());
			size_t totalExpected = headerEnd + 4 + contentLength;
			if (requestString.size() < totalExpected)
				return;
		}
	}
	Request req;
	Response res;
	parseHttpRequest(requestString, req, res);
	if (res.getStatus() == 400)
	{
		res.setHeader("Connection", "close");
		makeReadyforSend(res, client);
		return;
	}

	// Checking if the request contains a "Host" header and returning 'Bad Request' if not
	if (req.getHeaders().count("Host") == 0)
	{
		res.setStatus(400);
		res.setHeader("Connection", "close");
		makeReadyforSend(res, client);
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
		makeReadyforSend(res, client);
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
			makeReadyforSend(res, client);
			return;
		}

		// Refactored CGI handling
		if (handleCgiRequest(req, res, loc, client))
			return;
	}

	std::string method = req.getMethod();
	std::string path = req.getPath();
	std::string body = req.getBody();

	// Get allowed methods from the matched location
	const std::vector<std::string> &allowedMethods = loc->getMethods();

	// Check if the method is allowed
	if (std::find(allowedMethods.begin(), allowedMethods.end(), method) == allowedMethods.end())
	{
		// Method not allowed, respond with 405
		res.setStatus(405);

		// Add the Allow header with the permitted methods
		std::ostringstream allowHeader;
		for (size_t i = 0; i < allowedMethods.size(); ++i)
		{
			if (i > 0)
				allowHeader << ", ";
			allowHeader << allowedMethods[i];
		}
		res.setHeader("Allow", allowHeader.str());

		// Set the response body
		std::string html =
			"<html><head><title>405 Method Not Allowed</title></head><body>"
			"<h1>405 Method Not Allowed</h1>"
			"<p>The method " +
			method + " is not allowed for the requested resource.</p>"
					 "</body></html>";
		res.setHeader("Content-Type", "text/html");
		res.setHeader("Content-Length", intToStr(html.size()));
		res.setBody(html);

		makeReadyforSend(res, client);
		return;
	}

	if (method == "GET")
		handleGetRequest(res, req);
	else if (method == "POST")
		handlePostRequest(req, res, path, body);
	else if (method == "DELETE")
		handleDeleteRequest(res, path);

	makeReadyforSend(res, client);
}
