#include "../include/ServerConfig.hpp"
#include "../include/Utils.hpp"
#include <sstream>

// one of the important things is that the order here
// need to match the declaration order
ServerConfig::ServerConfig()
	: host("0.0.0.0"),
	  port(80),
	  server_name(""),
	  root("."),
	  index("index.html"),
	  client_max_body_size(1000000),
	  error_pages()
{
}


// this functions reads line by line the config file and extracts the
// first word then assigns the vealue for the class. If a location block
// is found it just creates a new object and parse the specific block.
// Last about the error page form the config file it saves it.
void ServerConfig::parseBlock(std::istream &stream)
{
	std::string line;
	while (std::getline(stream, line))
	{
		if (line.find('}') != std::string::npos)
			break;
		line = removeSemicolon(line);
		std::istringstream iss(line);
		std::string key;
		iss >> key;

		if (key == "listen")
			iss >> port;
		else if (key == "host")
			iss >> host;
		else if (key == "server_name")
			iss >> server_name;
		else if (key == "root")
			iss >> root;
		else if (key == "index")
			iss >> index;
		else if (key == "client_max_body_size")
			iss >> client_max_body_size;
		else if (key == "error_page")
		{
			int code;
			std::string page;
			iss >> code >> page;
			error_pages[code] = page;
		}
		else if (key == "location")
		{
			std::string path;
			iss >> path;
			LocationConfig loc;
			loc.setPath(path);
			loc.parseBlock(stream);
			locations.push_back(loc);
		}
	}
}

void ServerConfig::initialisedCheck() const {
	if (port == 0)
		throw std::runtime_error("Server port not set.");
	if (server_name.empty())
		throw std::runtime_error("Server name not set.");
	if (root.empty())
		throw std::runtime_error("Root directory not set.");
	if (client_max_body_size == 0)
		throw std::runtime_error("Max body size not set.");
	if (index.empty())
		throw std::runtime_error("Index file not set.");
	if (error_pages.empty())
		throw std::runtime_error("Error pages not set.");
}

const std::string& ServerConfig::getHost() const { return host; }
int ServerConfig::getPort() const { return port; }
const std::string& ServerConfig::getServerName() const { return server_name; }
const std::string& ServerConfig::getRoot() const { return root; }
const std::string& ServerConfig::getIndex() const { return index; }
size_t ServerConfig::getClientMaxBodySize() const { return client_max_body_size; }
const std::map<int, std::string>& ServerConfig::getErrorPages() const { return error_pages; }
const std::vector<LocationConfig>& ServerConfig::getLocations() const { return locations; }

const std::string& ServerConfig::getErrorPage(int code) const {
	static const std::string empty;
	std::map<int, std::string>::const_iterator it = error_pages.find(code);
	if (it != error_pages.end())
		return it->second;
	return empty;
}

void ServerConfig::print() const
{
	std::cout << "==== SERVER ====" << std::endl;
	std::cout	<< "Host: " << host
				<< "\nPort: " << port
				<< "\nRoot: " << root
				<< "\nIndex: " << index
				<< "\nServername: " << server_name
				<< "\nClient Max Body Size: " << client_max_body_size
				<< std::endl;
	std::cout << "Error pages:\n";
	for (std::map<int, std::string>::const_iterator it = error_pages.begin(); it != error_pages.end(); ++it)
		std::cout << "\tError: " << it->first << ", Path: " << it->second << std::endl;
	for (size_t i = 0; i < locations.size(); ++i)
		locations[i].print();
}

void print_servers(std::vector<ServerConfig>& servers)
{
	for (size_t i = 0; i < servers.size(); ++i)
		servers[i].print();
}
