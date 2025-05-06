#include "../include/LocationConfig.hpp"
#include "../include/Utils.hpp"
#include <sstream>

LocationConfig::LocationConfig() : autoindex(false)
{
	methods.push_back("GET");
	autoindex = false;
	root = "."; // or inherit from server block
	index = "index.html";
	cgi_path = "";
	cgi_ext = "";
	redirect = "";
}

void LocationConfig::parseBlock(std::istream &stream)
{
	std::string line;
	while (std::getline(stream, line))
	{
		if (line.find('}') != std::string::npos)
			break; // this is to be sure that it's the end of the block
		line = removeSemicolon(line);
		std::istringstream iss(line);
		std::string key;
		iss >> key;

		if (key == "root")
			iss >> root;
		else if (key == "index")
			iss >> index;
		else if (key == "autoindex")
		{
			std::string val;
			iss >> val;
			std::cout << "before: " << val << std::endl;
			autoindex = (val == "on");
			std::cout << val << std::endl;
		}
		else if (key == "allow_methods")
		{
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

const std::string &LocationConfig::getPath() const { return path; }
const std::string &LocationConfig::getRoot() const { return root; }
const std::string &LocationConfig::getIndex() const { return index; }
bool LocationConfig::isAutoindex() const { return autoindex; }
const std::vector<std::string> &LocationConfig::getMethods() const { return methods; }
const std::string &LocationConfig::getRedirect() const { return redirect; }
const std::string &LocationConfig::getCgiPath() const { return cgi_path; }
const std::string &LocationConfig::getCgiExt() const { return cgi_ext; }

void LocationConfig::setPath(const std::string &p) { path = p; }
void LocationConfig::setRoot(const std::string &r) { root = r; }
void LocationConfig::setIndex(const std::string &i) { index = i; }
void LocationConfig::setAutoindex(bool a) { autoindex = a; }
void LocationConfig::setMethods(const std::vector<std::string> &m) { methods = m; }
void LocationConfig::setRedirect(const std::string &r) { redirect = r; }
void LocationConfig::setCgiPath(const std::string &p) { cgi_path = p; }
void LocationConfig::setCgiExt(const std::string &e) { cgi_ext = e; }

void LocationConfig::print() const
{
	std::cout << "--- Location: " << path << " ---" << std::endl;
	std::cout << "Root: " << root
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
