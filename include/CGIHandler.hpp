#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <string>
#include <map>
#include "Request.hpp"
#include "LocationConfig.hpp"

class CGIHandler {
public:
	CGIHandler(const Request& req, const LocationConfig& loc);
	std::string run();
	bool wasSuccessful() const;
	std::string getError() const;
private:
	std::string scriptPath_;
	std::string interpreterPath_;
	std::map<std::string, std::string> env_;
	std::string requestBody_;
	bool success_;
	std::string errorMsg_;
};

#endif // CGI_HANDLER_HPP
