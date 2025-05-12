#pragma once

#include <string>
#include <map>
#include <sstream>

class Server;


class Response {
private:
	int _statusCode;
	std::map<std::string, std::string> _headers;
	std::string _body;

public:
	Response();
	void setStatus(int code);
	void setHeader(const std::string& key, const std::string& value);
	void setBody(const std::string& body);
	std::string toString() const;
	std::string getHeaderValue(const std::string& key) const;
	void sendResponse(Server& server, int clientFD, size_t index);
};

const char* getReasonPhrase(int code);