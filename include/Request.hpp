#include <string>
#include <map>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>

class Request {
public:
	std::string method;
	std::string path;
	std::string protocol;
	std::map<std::string, std::string> headers;
	std::string body;

	Request() {}
	void print() const;
};

Request parseHttpRequest(const std::string &rawRequest);
