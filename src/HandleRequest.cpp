#include "../include/Server.hpp"
#include "../include/Socket.hpp"
#include "../include/Request.hpp"
#include "../include/Webserver.hpp"
#include "../include/CGIHandler.hpp"
#include "../include/Logger.hpp"
#include "../include/Utils.hpp"

void Server::handleGetRequest(Response &res, const Request &req)
{
	const std::string &path = req.getPath();
	std::string fullPath = "www" + path;

	// Check if path is a directory
	if (isDirectory(fullPath))
	{
		// If it's a directory, check if we should serve directory listing
		const LocationConfig *loc = req.getMatchedLocation();
		if (loc && loc->isAutoindex())
		{
			logInfo("Directory listing requested: " + fullPath);
			list_directory(fullPath, res);
			return;
		}
		else
		{
			// Autoindex is disabled, return 403 Forbidden
			logWarning("403 Forbidden: Directory listing disabled for " + fullPath);
			std::string body = "<html><body><h1>403 Forbidden</h1><p>Directory listing is disabled.</p></body></html>";
			res.setStatus(403);
			res.setHeader("Content-Type", "text/html");
			res.setHeader("Content-Length", intToStr(body.size()));
			res.setBody(body);
			return;
		}
	}

	// Handle regular file request
	std::ifstream file(fullPath.c_str(), std::ios::binary);

	if (!file.is_open())
	{
		const ServerConfig *serverConfig = req.getServerConfig();
		if (serverConfig)
		{
			const std::string &errorPagePath = serverConfig->getErrorPage(404);
			if (!errorPagePath.empty())
			{
				std::string customErrorPage = "www/" + errorPagePath;
				std::ifstream errorFile(customErrorPage.c_str(), std::ios::binary);
				if (errorFile.is_open())
				{
					std::ostringstream content;
					content << errorFile.rdbuf();
					errorFile.close();
					res.setStatus(404);
					res.setHeader("Content-Type", "text/html");
					res.setHeader("Content-Length", intToStr(content.str().size()));
					res.setBody(content.str());
					return;
				}
			}
		}
	}
	else
	{
		std::ostringstream content;
		content << file.rdbuf();

		std::string type = getContentType(fullPath);
		logInfo("200 OK: " + fullPath + " (" + type + ")");
		res.setStatus(200);
		res.setHeader("Content-Type", type);
		res.setHeader("Content-Length", intToStr(content.str().size()));
		res.setBody(content.str());
	}
}

void Server::handlePostRequest(Request &req, Response &res, const std::string &path, const std::string &requestBody)
{
	std::string uploadDir = "www/upload/";

	// Check if this is a file upload (multipart/form-data)
	if (path == "/upload")
	{

		std::string boundary;
		std::string contentType = req.getHeader("Content-Type");
		size_t boundaryPos = contentType.find("boundary=");
		if (boundaryPos != std::string::npos)
		{
			boundary = "--" + contentType.substr(boundaryPos + 9);
		}
		if (boundary.empty())
		{
			res.setStatus(400);
			res.setBody("No boundary found in Content-Type");
			return;
		}

		// Find the start of the file content
		size_t fileStart = requestBody.find("\r\n\r\n");
		if (fileStart == std::string::npos)
		{
			res.setStatus(400);
			res.setBody("Malformed multipart body");
			return;
		}
		fileStart += 4; // Skip past the header

		// Find the end of the file content
		size_t fileEnd = requestBody.find(boundary, fileStart);
		if (fileEnd == std::string::npos)
		{
			res.setStatus(400);
			res.setBody("Malformed multipart body (no end boundary)");
			return;
		}
		std::string fileContent = requestBody.substr(fileStart, fileEnd - fileStart - 2); // -2 for \r\n

		// Extract filename from Content-Disposition
		size_t filenamePos = requestBody.find("filename=\"");
		if (filenamePos == std::string::npos)
		{
			res.setStatus(400);
			res.setBody("No filename found in multipart body");
			return;
		}
		filenamePos += 10;
		size_t filenameEnd = requestBody.find("\"", filenamePos);
		std::string filename = requestBody.substr(filenamePos, filenameEnd - filenamePos);

		std::string fullPath = uploadDir + filename;
		std::ofstream outFile(fullPath.c_str(), std::ios::binary);
		if (!outFile.is_open())
		{
			res.setStatus(500);
			res.setBody("Failed to open file for writing: " + fullPath);
			return;
		}
		outFile.write(fileContent.c_str(), fileContent.size());
		outFile.close();

		std::string body =
			"<html><body>"
			"<script>alert('File uploaded!'); window.location.href='/';</script>"
			"</body></html>";
		res.setStatus(200);
		res.setHeader("Content-Type", "text/html");
		res.setHeader("Content-Length", intToStr(body.size()));
		res.setBody(body);
		return;
	}

	// Fallback: normal POST (not file upload)
	std::string fullPath = "www" + path;
	std::ofstream outFile(fullPath.c_str());
	if (!outFile.is_open())
	{
		logError("Failed to open file for writing: " + fullPath);
		res.setStatus(500);
		std::string err = "Failed to open file for writing: " + fullPath;
		res.setHeader("Content-Type", "text/plain");
		res.setHeader("Content-Length", intToStr(err.size()));
		res.setBody(err);
		return;
	}

	if (outFile << requestBody)
		logInfo("POST request successful: Upload file has been filled: " + fullPath);

	outFile.close();
	std::string body =
		"<html><body>\n"
		"<h1>POST Received</h1>\n"
		"<br>\n"
		"<p>Path: " +
		path + "</p>\n"
			   "</body></html>";

	res.setStatus(200);
	res.setHeader("Content-Type", "text/html");
	res.setHeader("Content-Length", intToStr(body.size()));
	res.setBody(body);
}

void Server::handleDeleteRequest(Response &res, const std::string &path)
{
	std::string fullPath = "www" + path;
	std::ostringstream body;

	if (std::remove(fullPath.c_str()) == 0)
	{
		logInfo("File deleted successfully: " + fullPath);
		res.setStatus(200);
		body << "<html><body><h1>File Deleted</h1><p>Deleted: " << path << "</p></body></html>";
	}
	else
	{
		// File no found
		if (errno == ENOENT)
		{
			logWarning("404 Not Found for DELETE: " + fullPath);
			res.setStatus(404);
			body << "<html><body><h1>404 Not Found</h1><p>File not found: " << path << "</p></body></html>";
		}
		// Permission denied
		else if (errno == EACCES || errno == EPERM)
		{
			logError("403 Forbidden for DELETE: " + fullPath);
			res.setStatus(403);
			body << "Permission denied";
		}
		// Other errors
		else
		{
			logError("500 Internal Server Error for DELETE: " + fullPath + " - " + std::string(strerror(errno)));
			res.setStatus(500);
			body << "<html><body><h1>500 Internal Server Error</h1><p>Error deleting: " << path << "</p></body></html>";
		}
	}

	res.setBody(body.str());
	res.setHeader("Content-Type", "text/html");
	res.setHeader("Content-Length", intToStr(body.str().size()));
}
