#include "../include/CGIHandler.hpp"
#include "../include/Utils.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <vector>
#include <cstring>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <iostream>

CGIHandler::CGIHandler(const Request &req, const LocationConfig &loc)
	: success_(false)
{
	std::string locationRoot = loc.getRoot();
	std::string reqPath = req.getPath();
	if (!locationRoot.empty() && locationRoot[locationRoot.size() - 1] == '/')
		locationRoot = locationRoot.substr(0, locationRoot.size() - 1);
	if (!reqPath.empty() && reqPath[0] == '/')
		reqPath = reqPath.substr(1);

	scriptPath_ = locationRoot + "/" + reqPath;
	interpreterPath_ = loc.getCgiPath();
	std::string transferEncoding = req.getHeader("Transfer-Encoding");
	if (transferEncoding == "chunked")
	{
		std::istringstream rawStream(req.getBody());
		requestBody_ = decodeChunkedBody(rawStream);
	}
	else
		requestBody_ = req.getBody();
	setupEnvironment(req);
}

void CGIHandler::setupEnvironment(const Request &req)
{
	env_["REQUEST_METHOD"] = req.getMethod();
	env_["SCRIPT_NAME"] = req.getPath();
	env_["SERVER_PROTOCOL"] = req.getProtocol();
	env_["CONTENT_LENGTH"] = requestBody_.empty() ? "0" : intToStr(requestBody_.size());
	env_["PATH_INFO"] = req.getPath();

	std::map<std::string, std::string> headers = req.getHeaders();
	for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); ++it)
	{
		std::string key = it->first;
		for (std::size_t i = 0; i < key.size(); ++i)
			key[i] = std::toupper(key[i]);
		std::replace(key.begin(), key.end(), '-', '_');
		if (key == "CONTENT_TYPE")
			env_["CONTENT_TYPE"] = it->second;
		else
			env_["HTTP_" + key] = it->second;
	}

	if (req.getMethod() == "GET")
	{
		std::string query = extractQueryString(req.getPath());
		if (!query.empty())
			env_["QUERY_STRING"] = query;
	}
}

std::string CGIHandler::extractQueryString(const std::string &path)
{
	std::size_t pos = path.find('?');
	if (pos != std::string::npos)
		return path.substr(pos + 1);
	return "";
}

std::string CGIHandler::getExtension(const std::string &filename)
{
	std::size_t dot = filename.rfind('.');
	if (dot != std::string::npos)
		return filename.substr(dot);
	return "";
}

std::string CGIHandler::run()
{
	int inPipe[2], outPipe[2], errPipe[2];
	if (pipe(inPipe) == -1 || pipe(outPipe) == -1 || pipe(errPipe) == -1)
	{
		errorMsg_ = "Pipe creation failed";
		return "";
	}

	pid_t pid = fork();
	if (pid < 0)
	{
		errorMsg_ = "Fork failed";
		return "";
	}

	if (pid == 0)
	{
		dup2(inPipe[0], STDIN_FILENO);
		dup2(outPipe[1], STDOUT_FILENO);
		dup2(errPipe[1], STDERR_FILENO);

		close(inPipe[1]);
		close(outPipe[0]);
		close(errPipe[0]);

		std::vector<char *> envp;
		std::map<std::string, std::string>::const_iterator it;
		for (it = env_.begin(); it != env_.end(); ++it)
		{
			std::string entry = it->first + "=" + it->second;
			char *env_entry = strdup(entry.c_str());
			envp.push_back(env_entry);
		}
		envp.push_back(NULL);

		char *argv[3];
		argv[0] = const_cast<char *>(interpreterPath_.c_str());
		argv[1] = const_cast<char *>(scriptPath_.c_str());
		argv[2] = NULL;

		execve(interpreterPath_.c_str(), argv, &envp[0]);
		_exit(1);
	}
	else
	{
		close(inPipe[0]);
		close(outPipe[1]);
		close(errPipe[1]);

		if (!requestBody_.empty())
			write(inPipe[1], requestBody_.c_str(), requestBody_.size());
		close(inPipe[1]);

		std::stringstream output;
		char buf[4096];
		ssize_t n;
		while ((n = read(outPipe[0], buf, sizeof(buf))) > 0)
			output.write(buf, n);
		close(outPipe[0]);

		std::stringstream errOutput;
		while ((n = read(errPipe[0], buf, sizeof(buf))) > 0)
			errOutput.write(buf, n);
		close(errPipe[0]);

		int status;
		waitpid(pid, &status, 0);

		if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
		{
			success_ = true;
			return output.str();
		}
		else
		{
			errorMsg_ = "CGI script failed: " + errOutput.str();
			return "";
		}
	}
}

bool CGIHandler::wasSuccessful() const
{
	return success_;
}

std::string CGIHandler::getError() const
{
	return errorMsg_;
}
