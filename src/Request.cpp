#include "../include/Server.hpp"
#include "../include/Request.hpp"

Request parseHttpRequest(const std::string &rawRequest) {
	Request request;
	std::istringstream stream(rawRequest);
	std::string line;

	// Parse request line
	if (!std::getline(stream, line) || line.empty())
		throw std::runtime_error("Invalid HTTP request line");

	std::istringstream requestLine(line);
	requestLine >> request.method >> request.path >> request.protocol;

	// Parse headers
	while (std::getline(stream, line) && line != "\r") {
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		size_t colon = line.find(':');
		if (colon != std::string::npos) {
			std::string key = line.substr(0, colon);
			std::string value = line.substr(colon + 1);
			value.erase(0, value.find_first_not_of(" \t"));
			request.headers[key] = value;
		}
	}

	if (request.headers.count("Content-Length")) {
		int length = std::atoi(request.headers["Content-Length"].c_str());
		std::string body(length, '\0');
		stream.read(&body[0], length);
		request.body = body;
	}

	std::cout << "Received HTTP request from client " << ": "
		  << request.method << " " << request.path << " " << request.protocol << "\n";
	return request;
}

void Request::print() const {
	std::cout << "\n====== HTTP Request ======" << std::endl;

	std::cout << "Method   : " << method << std::endl;
	std::cout << "Path     : " << path << std::endl;
	std::cout << "Protocol : " << protocol << std::endl;

	std::cout << "\nHeaders:" << std::endl;
	std::map<std::string, std::string>::const_iterator it;
	for (it = headers.begin(); it != headers.end(); ++it) {
		std::cout << "  " << it->first << ": " << it->second << std::endl;
	}

	std::cout << "\nBody:" << std::endl;
	if (body.empty())
		std::cout << "  (empty)" << std::endl;
	else
		std::cout << body << std::endl;

	std::cout << "===========================\n" << std::endl;
}

void Server::handleGetRequest(int clientFd, const std::string &path)
{
	Response res;
	std::string fullPath = "www" + path;
	std::ifstream file(fullPath.c_str(), std::ios::binary);

	std::cout << "File opened successfully: " << fullPath << std::endl;

	if (!file.is_open())
	{
		std::string error = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404 Not Found";
		std::cout << "File not found: " << fullPath << "\nStatus Code: 404 Not Found\n";		send(clientFd, error.c_str(), error.size(), 0);
	}
	else
	{
		std::ostringstream content;
		content << file.rdbuf();
		// std::string body = content.str();
		std::string type = getContentType(fullPath);

		// std::ostringstream header;
		res.setStatus(200, "OK");
		res.setHeader("Content-Type", type);
		res.setHeader("Content-Length", intToStr(content.str().size()));
		res.setHeader("Connection", "close");
		res.setBody(content.str());
		// header << "HTTP/1.1 200 OK\r\n";
		// header << "Content-Type: " << type << "\r\n";
		// header << "Content-Length: " << body.size() << "\r\n";
		// header << "Connection: close\r\n\r\n";

		// std::string response = header.str() + body;
		std::string response = res.toString();
		send(clientFd, response.c_str(), response.size(), 0);
	}
	//std::cout << "Status Code: 200 OK" << std::endl;
	close(clientFd);
}

void Server::handlePostRequest(int clientFd, const std::string &path, const std::string &requestBody)
{
	//still not ready
	(void)requestBody;
	std::string response = "HTTP/1.1 201 Created\r\nContent-Type: text/plain\r\n\r\nPOST request received for: " + path;
	send(clientFd, response.c_str(), response.size(), 0);

	close(clientFd);
}

void Server::handleDeleteRequest(int clientFd, const std::string &path)
{
	//still not ready
	std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nDELETE request received for: " + path;
	send(clientFd, response.c_str(), response.size(), 0);

	close(clientFd);
}
