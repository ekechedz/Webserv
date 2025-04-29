#include "../include/Server.hpp"

void Server::handleGetRequest(int clientFd, const std::string &path)
{
	std::string fullPath = "www" + path;
	std::ifstream file(fullPath.c_str(), std::ios::binary);

	std::cout << "File opened successfully: " << fullPath << std::endl;

	if (!file.is_open())
	{
		std::string error = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404 Not Found";
		std::cout << "Status Code: 404 Not Found" << std::endl;
		send(clientFd, error.c_str(), error.size(), 0);
	}
	else
	{
		std::ostringstream content;
		content << file.rdbuf();
		std::string body = content.str();
		std::string type = getContentType(fullPath);

		std::ostringstream header;
		header << "HTTP/1.1 200 OK\r\n";
		header << "Content-Type: " << type << "\r\n";
		header << "Content-Length: " << body.size() << "\r\n";
		header << "Connection: close\r\n\r\n";

		std::string response = header.str() + body;
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
