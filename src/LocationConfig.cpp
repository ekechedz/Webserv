#include "../include/LocationConfig.hpp"
#include <sstream>

LocationConfig::LocationConfig() : autoindex(false)
{
	methods.push_back("GET");
	autoindex = false;
	root = ".";	//or inherit from server block
	index = "index.html";
	cgi_path = "";
	cgi_ext = "";
	redirect = "";
}

void LocationConfig::parseBlock(std::istream &stream) {
	std::string line;
	while (std::getline(stream, line)) {
		if (line.find('}') != std::string::npos)
			break; // this is to be sure that it's the end of the block
		std::istringstream iss(line);
		std::string key;
		iss >> key;

		if (key == "root")
			iss >> root;
		else if (key == "index")
			iss >> index;
		else if (key == "autoindex") {
			std::string val;
			iss >> val;
			autoindex = (val == "on");
		}
		else if (key == "allow_methods") {
			methods.clear();
			std::string method;
			while (iss >> method)
				methods.push_back(method);
		}
		else if (key == "redirect")
			iss >> redirect;
		else if (key == "cgi_path")
			iss >> cgi_path;
		else if (key == "cgi_ext")
			iss >> cgi_ext;
	}
}

void LocationConfig::print() const {
	std::cout << "--- Location: " << path << " ---" << std::endl;
	std::cout	<< "Root: " << root
				<< "\nIndex: " << index
				<< "\nAutoindex: " << (autoindex ? "on" : "off")
				<< "\nRedirect: " << redirect
				<< "\nCGI Path: " << cgi_path
				<< "\nCGI Ext: " << cgi_ext << std::endl;
	std::cout << "Methods: ";
	for (size_t i = 0; i < methods.size(); ++i)
		std::cout << methods[i] << " ";
	std::cout << std::endl;
}
