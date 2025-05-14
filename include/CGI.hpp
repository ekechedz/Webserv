#pragma once

#include <string>
#include <map>
#include <vector>
#include <cstdio>
#include "Request.hpp"

class CGI {
private:
	std::string scriptPath;
	std::string interpreter;
	std::map<std::string, std::string> env;
	std::string requestBody;
	std::vector<std::string> args;

public:
	CGI(const std::string &scriptPath, const std::string &interpreter);

	void setEnvVar(const std::string &key, const std::string &value);
	void setRequestBody(const std::string &body);
	void setArgs(const std::vector<std::string> &arguments);

	// Getters and setters
	const std::string& getScriptPath() const;
	void setScriptPath(const std::string &path);

	const std::string& getInterpreter() const;
	void setInterpreter(const std::string &interp);

	const std::map<std::string, std::string>& getEnv() const;
	void setEnv(const std::map<std::string, std::string> &environment);

	const std::string& getRequestBody() const;
	const std::vector<std::string>& getArgs() const;

	// Setup from Request object
	void setupFromRequest(const Request& req);

	std::string execute();
};
