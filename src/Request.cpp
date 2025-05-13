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
		res.setHeader("Connection", "close");
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
	res.setHeader("Connection", "close");
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
	res.setHeader("Connection", "close");
}
