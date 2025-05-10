#include "../include/Utils.hpp"

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
	if (path.size() >= 4 && path.substr(path.size() - 4) == ".ico")
		return "image/x-icon";
	return "application/octet-stream";
}
int printError(const std::string &msg, int exitCode)
{
	std::cerr << "\033[1;31m[ERROR] " << msg << "\033[0m" << std::endl;
	return exitCode;
}

// Wraps std::getline(). Throws if line does not contain \r. Returns line without \r.
std::string getlineCarriage(std::istringstream& iss)
{
	std::string line;
	if (!std::getline(iss, line))
		throw std::runtime_error("Get line failed.");
	if (line.empty() || line[line.size() - 1] != '\r')
		throw std::runtime_error("New line without carriage return.");
	line.erase(line.size() - 1);
	return line;
}

// Trims leading and trailing whitespace
std::string trim(const std::string& str)
{
	std::string::size_type start = 0;
	std::string::size_type end = str.length();

	// Find the first non-whitespace character from the beginning
	while (start < end && std::isspace(static_cast<unsigned char>(str[start]))) {
		++start;
	}

	// Find the last non-whitespace character from the end
	while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
		--end;
	}

	return str.substr(start, end - start);
}
