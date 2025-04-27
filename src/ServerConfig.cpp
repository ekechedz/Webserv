#include "../include/ServerConfig.hpp"
#include <sstream>

// one of the important things is that the order here
// need to match the declaration order

ServerConfig::ServerConfig()
	: host(""),
	  port(0),
	  server_name(""),
	  root(""),
	  index(""),
	  client_max_body_size()
{
}
std::string removeSemicolon(const std::string &str)
{
	std::string line = str;

	size_t commentPos = line.find_first_of("//#");

	if (commentPos != std::string::npos)
		line = line.substr(0, commentPos);

	if (!line.empty() && line[line.size() - 1] == ';')
		line = line.substr(0, line.size() - 1);
	size_t start = line.find_first_not_of(" \t");
	size_t end = line.find_last_not_of(" \t");
	if (start == std::string::npos)
		return "";
	return line.substr(start, end - start + 1);
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
			loc.path = path;
			loc.parseBlock(stream);
			locations.push_back(loc);
		}
	}
}

void ServerConfig::print() const
{
	std::cout << "==== SERVER ====" << std::endl;
	std::cout << "Host: " << host << "\nPort: " << port << "\nRoot: " << root << "\nIndex: " << index << std::endl;
	for (size_t i = 0; i < locations.size(); ++i)
	{
		locations[i].print();
	}
}
