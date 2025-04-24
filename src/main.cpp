#include "../include/Webserver.hpp"

int printError(const std::string &msg, int exitCode = 1)
{
	std::cerr << "\033[1;31m[ERROR] " << msg << "\033[0m" << std::endl;
	return exitCode;
}

int main(int argc, char **argv)
{
	try
	{
		if (argc != 2)
			throw std::invalid_argument("Usage: ./webserv <config_file>.conf");
		signal(SIGPIPE, SIG_IGN); // this prevents the server from crashing e.g when someone just close the tab
		std::string configFile = (argv[1]);
		Parser parsServ;
		parsServ.parseServerConfig(configFile);
		// starting
		// connect  website server
	}
	catch (std::exception &err)
	{
		printError(err.what(), ERR_CONFIG_FAIL);
	}
}
