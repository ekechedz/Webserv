#pragma once // i am not sure if we are allowed to use this because in the exam u can but fort the project idk
#include <string>
#include <vector>
#include <iostream>

class LocationConfig {
public:
	std::string path;
	std::string root;
	std::string index;
	bool autoindex;
	std::vector<std::string> methods;
	std::string redirect;
	std::string cgi_path;
	std::string cgi_ext;

	LocationConfig();
	void parseBlock(std::istream &stream);
	void print() const;
};
