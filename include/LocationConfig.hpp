#pragma once // i am not sure if we are allowed to use this because in the exam u can but fort the project idk
#include <string>
#include <vector>
#include <iostream>

class LocationConfig {
private:
	std::string path;
	std::string root;
	std::string index;
	bool autoindex;
	std::vector<std::string> methods;
	std::string redirect;
	std::string cgi_path;
	std::string cgi_ext;
public:

	LocationConfig();
	void parseBlock(std::istream &stream);
	void print() const;

	const std::string& getPath() const;
	const std::string& getRoot() const;
	const std::string& getIndex() const;
	bool isAutoindex() const;
	const std::vector<std::string>& getMethods() const;
	const std::string& getRedirect() const;
	const std::string& getCgiPath() const;
	const std::string& getCgiExt() const;

	void setPath(const std::string& p);
	void setRoot(const std::string& r);
	void setIndex(const std::string& i);
	void setAutoindex(bool a);
	void setMethods(const std::vector<std::string>& m);
	void setRedirect(const std::string& r);
	void setCgiPath(const std::string& p);
	void setCgiExt(const std::string& e);
};
