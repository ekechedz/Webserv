#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <string>
#include <map>
#include "Request.hpp"
#include "LocationConfig.hpp"

class CGIHandler {
public:
	CGIHandler(const Request& req, const LocationConfig& loc);
	void handleFileUpload(const std::string& body, const std::string& uploadDir);
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

	void setupEnvironment(const Request &req);
	std::string extractQueryString(const std::string &path);
	std::string getExtension(const std::string &filename);
};

#endif // CGI_HANDLER_HPP
