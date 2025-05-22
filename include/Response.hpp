#pragma once

#include <string>
#include <map>
#include <sstream>

class Server;


class Response
{
private:
	int _statusCode;
	std::map<std::string, std::string> _headers;
	std::string _body;

public:
	Response();
	void setStatus(int code);
	int getStatus() const;
	void setHeader(const std::string &key, const std::string &value);
	void setBody(const std::string &body);
	void setError(int code, const std::string& message);
	void setWarning(const std::string& message);
	std::string toString() const;
	std::string getHeaderValue(const std::string &key) const;
	void parseCgiOutput(const std::string &cgiOutput);
};

const char *getReasonPhrase(int code);
