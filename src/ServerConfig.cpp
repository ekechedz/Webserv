#include "../include/ServerConfig.hpp"
#include <sstream>

// one of the important things is that the order here
// need to match the declaration order

ServerConfig::ServerConfig()
	: host("127.0.0.1"),
	  port(8080),
	  server_name("localhost"),
	  root("./www/"),
	  index("index.html"),
	  client_max_body_size(3000000)
{}

// this functions reads line by line the config file and extracts the
// first word then assigns the vealue for the class. If a location block
// is found it just creates a new object and parse the specific block.
// Last about the error page form the config file it saves it.
void ServerConfig::parseBlock(std::istream &stream)
{
	std::string line;
	while (std::getline(stream, line)) {
		if (line.find('}') != std::string::npos)
			break;
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
		else if (key == "error_page") {
			int code;
			std::string page;
			iss >> code >> page;
			error_pages[code] = page;
		}
		else if (key == "location") {
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
	for (size_t i = 0; i < locations.size(); ++i) {
		locations[i].print();
	}
}

