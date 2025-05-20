#pragma once

#include <string>
#include <map>

class LocationConfig;
class ServerConfig;

class Request {
private:
	std::string method;
	std::string path;
	std::string protocol;
	std::map<std::string, std::string> headers;
	std::string body;
	const LocationConfig* matchedLocation;
	ServerConfig* serverConfig;

public:
	Request();

	std::string getMethod() const;
	std::string getPath() const;
	std::string getProtocol() const;
	std::map<std::string, std::string> getHeaders() const;
	std::string getBody() const;
	const LocationConfig* getMatchedLocation() const;
	const ServerConfig* getServerConfig() const;
	std::string getHeader(const std::string &key) const;

	void setMethod(const std::string& m);
	void setPath(const std::string& p);
	void setProtocol(const std::string& pr);
	void setHeaders(const std::map<std::string, std::string>& h);
	void setBody(const std::string& b);
	void setMatchedLocation(const LocationConfig* loc);
	void setServerConfig(ServerConfig* config);

	void print() const;
};

Request parseHttpRequest(const std::string &rawRequest);
