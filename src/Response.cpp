#include "../include/Response.hpp"

Response::Response() : _statusCode(200), _statusMessage("OK") {}

void Response::setStatus(int code, const std::string& message) {
	_statusCode = code;
	_statusMessage = message;
}

void Response::setHeader(const std::string& key, const std::string& value) {
	_headers[key] = value;
}

void Response::setBody(const std::string& body) {
	_body = body;
}

std::string Response::toString() const {
	std::ostringstream response;
	response << "HTTP/1.1 " << _statusCode << " " << _statusMessage << "\r\n";
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
		response << it->first << ": " << it->second << "\r\n";
	}
	response << "Content-Length: " << _body.size() << "\r\n";
	response << "\r\n";
	response << _body;
	return response.str();
}
