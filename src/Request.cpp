#include "../include/Request.hpp"

Request::Request(const ServerConfig& setServerConfig)
: serverConfig(setServerConfig)
{}

// Takes char* and extracts structured info. Throws if request contains syntax error.
void Request::parse(char* buffer)
{

	std::istringstream iss((std::string(buffer)));
	std::string line;
	
	// Parse request line (=first line)
	{
		line = getlineCarriage(iss);
		std::istringstream lineStream(line);
		// Extracts all three elements at once. And throws if something failed.
		if (!(lineStream >> method >> path >> protocol))
			throw std::runtime_error("Syntax error in request line.");
		// Check if there is a fourth element
		std::string extra;
		if (lineStream >> extra)
		{
			std::cout << "extra: " << "|" << extra << "|";
			throw std::runtime_error("Too many args in request line.");
		}
	}
	
	// Parse headers
	// Loops until it finds mandatory empty line after headers
	for (line = getlineCarriage(iss); !line.empty(); line = getlineCarriage(iss))
	{
		// Checking for colon
		std::size_t colonPos = line.find(':');
		if (colonPos == std::string::npos)
			throw std::runtime_error("No colon in header line.");
		// Splitting at colon and removing whitespace
		std::string key = trim(line.substr(0, colonPos));
		std::string value = trim(line.substr(colonPos + 1));
		// Adding to map of headers
		headers[key] = value;
	}

	// Copy body
	{
	std::ostringstream oss;
	oss << iss.rdbuf();
	body = oss.str();
	}
	(void)serverConfig;
}

void Request::print() {
	std::cout << "Request:" << std::endl;
	std::cout << "\tMethod: " << method << std::endl;
	std::cout << "\tPath: " << path << std::endl;
	std::cout << "\tProtocol: " << protocol << std::endl;
	std::cout << "\tHeaders:" << std::endl;
	
	std::map<std::string, std::string>::const_iterator it;
	for (it = headers.begin(); it != headers.end(); ++it) {
		std::cout << "\t\t" << it->first << ": " << it->second << std::endl;
	}
	
	std::cout << "\tBody:" << std::endl << body << std::endl;
}