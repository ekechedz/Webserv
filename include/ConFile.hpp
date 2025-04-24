#ifndef CONFIGFILE_HPP
#define CONFIGFILE_HPP

#include <string>
#include <sys/stat.h> // for stat(), S_ISREG, S_ISDIR

enum FileType {
    INVALID_TYPE = -1,
    FILE_TYPE = 1,
    DIRECTORY_TYPE = 2,
    OTHER_TYPE = 3
};

class ConfigFile
{
public:

    ConfigFile();
	ConfigFile(const std::string& path);
    ~ConfigFile();

    int getTypePath() const;


private:
	std::string	path;
};

#endif
