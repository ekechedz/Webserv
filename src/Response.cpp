#include "../include/Response.hpp"
#include "../include/Server.hpp"

Response::Response() : _statusCode(0) {}

void Response::setStatus(int code)
{
	_statusCode = code;
}

void Response::setHeader(const std::string &key, const std::string &value)
{
	_headers[key] = value;
}

void Response::setBody(const std::string &body)
{
	_body = body;
}
int Response::getStatus() const
{
	return _statusCode;
}
std::string Response::toString() const
{
	std::ostringstream response;
	response << "HTTP/1.1 " << _statusCode << " " << getReasonPhrase(_statusCode) << "\r\n";
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
	{
		response << it->first << ": " << it->second << "\r\n";
	}
	response << "Content-Length: " << _body.size() << "\r\n";
	response << "\r\n";
	response << _body;
	return response.str();
}

void Response::parseCgiOutput(const std::string &cgiOutput) {
	std::istringstream stream(cgiOutput);
	std::string line;
	bool headersDone = false;
	std::ostringstream body;
	while (std::getline(stream, line)) {
		if (!headersDone) {
			if (line == "\r" || line == "" || line == "\n") {
				headersDone = true;
				continue;
			}
			size_t colon = line.find(":");
			if (colon != std::string::npos) {
				std::string key = line.substr(0, colon);
				std::string value = line.substr(colon + 1);
				value.erase(0, value.find_first_not_of(" \t"));
				setHeader(key, value);
			}
		} else {
			body << line << "\n";
		}
	}
	std::string b = body.str();
	if (!b.empty() && b[b.size()-1] == '\n') b.erase(b.size()-1);
	setBody(b);
}

#define HTTP_STATUS_CODES            \
	/* Successful (2xx): */          \
	X(200, "OK")                     \
	X(201, "Created")                \
	X(204, "No Content")             \
	/* Redirection (3xx): */         \
	X(301, "Moved Permanently")      \
	X(302, "Found")                  \
	X(303, "See Other")              \
	X(307, "Temporary Redirect")     \
	X(308, "Permanent Redirect")     \
	/* Client Errors (4xx): */       \
	X(400, "Bad Request")            \
	X(403, "Forbidden")              \
	X(404, "Not Found")              \
	X(405, "Method Not Allowed")     \
	X(413, "Payload Too Large")      \
	X(415, "Unsupported Media Type") \
	X(418, "I'm a teapot")           \
	X(429, "Too Many Requests")      \
	/* Server Errors (5xx): */       \
	X(500, "Internal Server Error")  \
	X(501, "Not Implemented")        \
	X(502, "Bad Gateway")            \
	X(503, "Service Unavailable")    \
	X(504, "Gateway Timeout")		\
	X(505, "HTTP Version Not Supported")

const char *getReasonPhrase(int code)
{
	switch (code)
	{
#define X(num, text) \
	case num:        \
		return text;
		HTTP_STATUS_CODES
#undef X
	default:
		return "Unknown Status";
	}
}

std::string Response::getHeaderValue(const std::string &key) const
{
	std::map<std::string, std::string>::const_iterator it = _headers.find(key);
	if (it != _headers.end())
	{
		return it->second;
	}
	return "";
}
