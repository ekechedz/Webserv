#include "../include/ConFile.hpp"

ConfigFile::ConfigFile() : path("") {}

ConfigFile::ConfigFile(const std::string& path) : path(path) {}

int ConfigFile::getTypePath() const
{
	struct stat fileInfo;

	if (stat(path.c_str(), &fileInfo) != 0)
		return INVALID_TYPE;

	if (S_ISREG(fileInfo.st_mode))
		return FILE_TYPE;

	if (S_ISDIR(fileInfo.st_mode))
		return DIRECTORY_TYPE;

	return OTHER_TYPE;
}
