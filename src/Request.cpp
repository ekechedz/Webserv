#include "../include/Server.hpp"
#include "../include/Request.hpp"
#include "../include/Logger.hpp"
#include "../include/Utils.hpp"
#include <cstdlib>

Request::Request() : matchedLocation(NULL){}

std::string Request::getMethod() const { return method; }
std::string Request::getPath() const { return path; }
std::string Request::getProtocol() const { return protocol; }
std::map<std::string, std::string> Request::getHeaders() const { return headers; }
std::string Request::getBody() const { return body; }
const LocationConfig* Request::getMatchedLocation() const { return matchedLocation; }
const ServerConfig* Request::getServerConfig() const { return serverConfig; }

void Request::setMethod(const std::string& m) { method = m; }
void Request::setPath(const std::string& p) { path = p; }
void Request::setProtocol(const std::string& pr) { protocol = pr; }
void Request::setHeaders(const std::map<std::string, std::string>& h) { headers = h; }
void Request::setBody(const std::string& b) { body = b; }
void Request::setMatchedLocation(const LocationConfig* loc) { matchedLocation = loc; }
void Request::setServerConfig(ServerConfig* config) { serverConfig = config; }

std::string Request::getHeader(const std::string &key) const {
	const std::map<std::string, std::string>& hdrs = getHeaders();
	std::map<std::string, std::string>::const_iterator it = hdrs.find(key);
	if (it != hdrs.end())
		return it->second;
	return "";
}

void parseHttpRequest(const std::string &rawRequest, Request& request, Response& res)
{
	std::istringstream stream(rawRequest);
	std::string line;

	// Parse request line
	if (!std::getline(stream, line) || line.empty())
	{
		logError("Invalid HTTP request line");
		res.setStatus(400);
		return;
	}

	std::istringstream requestLine(line);
	std::string method, path, protocol;
	requestLine >> method >> path >> protocol;
	
	if (method.empty() || path.empty() || protocol.empty()) {
		logError("Invalid HTTP request format");
		res.setStatus(400);
		return;
	}
	
	request.setMethod(method);
	request.setPath(path);
	// TODO: Query string handling
	request.setProtocol(protocol);

	logInfo("Received request: " + method + " " + path + " " + protocol);

	// Parse headers
	std::map<std::string, std::string> headers;
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
			headers[key] = value;
		}
		else if (!line.empty())
		{
			logError("Invalid header format: " + line);
			res.setStatus(400);
			return;
		}
	}
	request.setHeaders(headers);

	if (headers.count("Transfer-Encoding") &&
		headers["Transfer-Encoding"] == "chunked")
	{
		try {
			request.setBody(decodeChunkedBody(stream));
		} catch (const std::exception& e) {
			logError("Error decoding chunked body: " + std::string(e.what()));
			res.setStatus(400);
			return;
		}
	}
	else if (headers.count("Content-Length"))
	{
		int length;
		try {
			length = std::atoi(headers["Content-Length"].c_str());
			if (length < 0) {
				logError("Invalid Content-Length value: " + headers["Content-Length"]);
				res.setStatus(400);
				return;
			}
		} catch (const std::exception& e) {
			logError("Error parsing Content-Length: " + std::string(e.what()));
			res.setStatus(400);
			return;
		}
		
		std::string body(length, '\0');
		stream.read(&body[0], length);
		if (stream.gcount() != length) {
			logError("Body size doesn't match Content-Length header");
			res.setStatus(400);
			return;
		}
		request.setBody(body);
	}
}
void Request::print() const
{
	std::cout << "\n====== HTTP Request ======" << std::endl;

	std::cout << "Method   : " << getMethod() << std::endl;
	std::cout << "Path     : " << getPath() << std::endl;
	std::cout << "Protocol : " << getProtocol() << std::endl;

	std::cout << "\nHeaders:" << std::endl;
	std::map<std::string, std::string> hdrs = getHeaders();
	for (std::map<std::string, std::string>::const_iterator it = hdrs.begin(); it != hdrs.end(); ++it)
		std::cout << "  " << it->first << ": " << it->second << std::endl;

	std::cout << "\nBody:" << std::endl;
	if (getBody().empty())
		std::cout << "  (empty)" << std::endl;
	else
		std::cout << getBody() << std::endl;

	std::cout << "===========================\n" << std::endl;
}
