#include "../include/Utils.hpp"
#include "../include/Logger.hpp"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <sys/stat.h>

std::string removeSemicolon(const std::string &str)
{
	std::string line = str;

	if (!line.empty() && line[line.size() - 1] == ';')
		line = line.substr(0, line.size() - 1);
	size_t start = line.find_first_not_of(" \t");
	size_t end = line.find_last_not_of(" \t");
	if (start == std::string::npos)
		return "";
	return line.substr(start, end - start + 1);
}
// its just when you get into the www folder to know what to do whit each file
std::string getContentType(const std::string &path)
{
	if (path.size() >= 5 && path.substr(path.size() - 5) == ".html")
		return "text/html";
	if (path.size() >= 4 && path.substr(path.size() - 4) == ".htm")
		return "text/html";
	if (path.size() >= 4 && path.substr(path.size() - 4) == ".css")
		return "text/css";
	if (path.size() >= 3 && path.substr(path.size() - 3) == ".js")
		return "application/javascript";
	if (path.size() >= 4 && path.substr(path.size() - 4) == ".png")
		return "image/png";
	if (path.size() >= 4 && path.substr(path.size() - 4) == ".jpg")
		return "image/jpeg";
	if (path.size() >= 5 && path.substr(path.size() - 5) == ".jpeg")
		return "image/jpeg";
	return "application/octet-stream";
}
int printError(const std::string &msg, int exitCode)
{
	std::cerr << "\033[1;31m[ERROR] " << msg << "\033[0m" << std::endl;
	return exitCode;
}
std::string intToStr(int num)
{
	std::ostringstream oss;
	oss << num;
	return oss.str();
}

std::string decodeChunkedBody(std::istream &stream)
{
	std::string decodedBody;
	std::string line;

	while (std::getline(stream, line))
	{

		if (!line.empty() && line[line.size() - 1] == '\r')
			line = line.substr(0, line.size() - 1);

		std::istringstream chunkSizeStream(line);
		size_t chunkSize;
		chunkSizeStream >> std::hex >> chunkSize;

		if (chunkSize == 0)
			break;

		std::string chunk(chunkSize, '\0');
		stream.read(&chunk[0], chunkSize);
		decodedBody += chunk;

		char cr, lf;
		stream.get(cr);
		stream.get(lf);
		if (cr != '\r' || lf != '\n')
			break;
	}

	return decodedBody;
}

std::string decodeEvents(short int events)
{
	std::string result;
	if (events & POLLIN)
		result += "POLLIN ";
	if (events & POLLOUT)
		result += "POLLOUT ";
	if (events & POLLERR)
		result += "POLLERR ";
	if (events & POLLHUP)
		result += "POLLHUP ";
	if (events & POLLNVAL)
		result += "POLLNVAL ";
	return result;
}

// Logging utility functions
void logError(const std::string &msg)
{
	Logger::getInstance().log(Logger::ERROR, msg);
}

void logWarning(const std::string &msg)
{
	Logger::getInstance().log(Logger::WARNING, msg);
}

void logInfo(const std::string &msg)
{
	Logger::getInstance().log(Logger::INFO, msg);
}

void logDebug(const std::string &msg)
{
	Logger::getInstance().log(Logger::DEBUG, msg);
}

// Directory listing utility functions
bool isDirectory(const std::string &path)
{
	struct stat statbuf;
	if (stat(path.c_str(), &statbuf) != 0)
		return false;
	return S_ISDIR(statbuf.st_mode);
}
