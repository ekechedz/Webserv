#include "../include/Server.hpp"
#include "../include/Request.hpp"

Request parseHttpRequest(const std::string &rawRequest)
{
	Request request;
	std::istringstream stream(rawRequest);
	std::string line;

	// Parse request line
	if (!std::getline(stream, line) || line.empty())
		throw std::runtime_error("Invalid HTTP request line");

	std::istringstream requestLine(line);
	requestLine >> request.method >> request.path >> request.protocol;

	// Parse headers
	while (std::getline(stream, line) && line != "\r")
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		size_t colon = line.find(':');
		if (colon != std::string::npos)
		{
			std::string key = line.substr(0, colon);
			std::string value = line.substr(colon + 1);
			value.erase(0, value.find_first_not_of(" \t"));
			request.headers[key] = value;
		}
	}
	if (request.headers.count("Transfer-Encoding") &&
		request.headers["Transfer-Encoding"] == "chunked")
		request.body = decodeChunkedBody(stream);
	else if (request.headers.count("Content-Length"))
	{
		int length = std::atoi(request.headers["Content-Length"].c_str());
		std::string body(length, '\0');
		stream.read(&body[0], length);
		request.body = body;
	}



	std::cout << "Received HTTP request from client " << ": "
			  << request.method << " " << request.path << " " << request.protocol << "\n";
	return request;
}

void Request::print() const
{
	std::cout << "\n====== HTTP Request ======" << std::endl;

	std::cout << "Method   : " << method << std::endl;
	std::cout << "Path     : " << path << std::endl;
	std::cout << "Protocol : " << protocol << std::endl;

	std::cout << "\nHeaders:" << std::endl;
	std::map<std::string, std::string>::const_iterator it;
	for (it = headers.begin(); it != headers.end(); ++it)
		std::cout << "  " << it->first << ": " << it->second << std::endl;

	std::cout << "\nBody:" << std::endl;
	if (body.empty())
		std::cout << "  (empty)" << std::endl;
	else
		std::cout << body << std::endl;

	std::cout << "===========================\n"
			  << std::endl;
}


