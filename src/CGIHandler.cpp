#include "../include/CGIHandler.hpp"
#include "../include/Utils.hpp"
#include "../include/Server.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <vector>
#include <cstring>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include "../include/Logger.hpp"

#include <fstream>

void CGIHandler::handleFileUpload(const std::string &body, const std::string &uploadDir)
{
	// Extract boundary from Content-Type header
	std::string contentType = env_["CONTENT_TYPE"];
	std::string boundary;
	std::size_t boundaryPos = contentType.find("boundary=");
	if (boundaryPos != std::string::npos)
	{
		boundary = "--" + contentType.substr(boundaryPos + 9); // "boundary=" is 9 characters
	}
	else
	{
		logError("Boundary not found in Content-Type header");
		return;
	}

	// Split the body into parts using the boundary
	std::string::size_type start = 0, end;
	while ((end = body.find(boundary, start)) != std::string::npos)
	{
		std::string part = body.substr(start, end - start);
		start = end + boundary.length();

		// Skip empty parts or the final boundary marker
		if (part.empty() || part == "--")
		{
			continue;
		}

		// Extract headers and body of the part
		std::size_t headerEnd = part.find("\r\n\r\n");
		if (headerEnd == std::string::npos)
		{
			logError("Malformed multipart/form-data part");
			continue;
		}

		std::string headers = part.substr(0, headerEnd);
		std::string fileContent = part.substr(headerEnd + 4); // Skip "\r\n\r\n"

		// Extract filename from Content-Disposition header
		std::string filename;
		std::size_t filenamePos = headers.find("filename=\"");
		if (filenamePos != std::string::npos)
		{
			filenamePos += 10; // Move past "filename=\""
			std::size_t endPos = headers.find("\"", filenamePos);
			if (endPos != std::string::npos)
			{
				filename = headers.substr(filenamePos, endPos - filenamePos);
			}
			else
			{
				logError("Malformed Content-Disposition header");
				continue;
			}
		}
		else
		{
			logError("Filename not found in Content-Disposition header");
			continue;
		}

		// Save the file to the upload directory
		std::string filePath = uploadDir + "/" + filename;
		std::ofstream outFile(filePath.c_str(), std::ios::binary); // Use .c_str() for C++98 compatibility
		if (!outFile)
		{
			logError("Failed to open file for writing: " + filePath);
			continue;
		}
		outFile.write(fileContent.c_str(), fileContent.size());
		outFile.close();

		logInfo("File uploaded successfully: " + filePath);
	}
}

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

	logInfo("CGI Request: " + req.getMethod() + " " + scriptPath_);

	std::string transferEncoding = req.getHeader("Transfer-Encoding");
	if (transferEncoding == "chunked")
	{
		std::istringstream rawStream(req.getBody());
		requestBody_ = decodeChunkedBody(rawStream);
	}
	else
		requestBody_ = req.getBody();

	std::string contentType = req.getHeader("Content-Type");
	if (req.getMethod() == "POST" && contentType.find("multipart/form-data") != std::string::npos)
	{
		std::string uploadDir = loc.getUploadDir();
		handleFileUpload(requestBody_, uploadDir);
	}
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
		logError(errorMsg_);
		return "";
	}

	pid_t pid = fork();
	if (pid < 0)
	{
		errorMsg_ = "Fork failed";
		logError(errorMsg_);
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
		for (std::map<std::string, std::string>::const_iterator it = env_.begin(); it != env_.end(); ++it)
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
		logError("CGI execve failed: " + std::string(strerror(errno)));
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


		int status;
		int timeout = 5; // Timeout in seconds
		pid_t result;
		time_t startTime = time(NULL);

		do
		{
			result = waitpid(pid, &status, WNOHANG);
			if (result == 0) // Child is still running
			{
				if (time(NULL) - startTime >= timeout)
				{
					kill(pid, SIGKILL); // Terminate the child process
					logError("CGI script timed out");
					errorMsg_ = "CGI script timed out";
					return "";
				}
				usleep(100000); // Sleep for 100ms before checking again
			}
		} while (result == 0);

		if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
		{
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

			success_ = true;
			logInfo("CGI script executed successfully: " + scriptPath_);
			return output.str();
		}
		else
		{
			std::stringstream errOutput;
			char buf[4096];
			ssize_t n;
			while ((n = read(errPipe[0], buf, sizeof(buf))) > 0)
				errOutput.write(buf, n);
			close(errPipe[0]);

			errorMsg_ = "CGI script failed: " + errOutput.str();
			logError(errorMsg_ + " for script: " + scriptPath_);
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

bool Server::handleCgiRequest(const Request &req, Response &res, const LocationConfig *loc, Socket &client)
{
	if (!loc || loc->getCgiPath().empty() || loc->getCgiExt().empty())
		return false;
	std::string ext = req.getPath().substr(req.getPath().find_last_of("."));
	if (ext != loc->getCgiExt())
		return false;

	logInfo("Processing CGI request: " + req.getPath());
	CGIHandler cgi(req, *loc);
	std::string cgiOutput = cgi.run();
	if (cgi.wasSuccessful())
	{
		logInfo("CGI execution successful: " + req.getPath());
		res.setStatus(200);
		res.parseCgiOutput(cgiOutput);
	}
	else
	{
		logError("CGI execution failed: " + cgi.getError());
		res.setStatus(500);
		res.setHeader("Content-Type", "text/plain");
		res.setBody("CGI execution failed: " + cgi.getError());
	}

	// Check if connection will close before sending response
	bool shouldClose = (res.getHeaderValue("Connection") == "close");

	// Clear buffer before potentially deleting the client
	if (!shouldClose)
	{
		client.clearBuffer();
	}

	sendResponse(res, client);
	return true;
}
