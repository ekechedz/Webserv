#include <iostream>
#include <stdexcept>
#include <vector>
#include <pthread.h>
#include <signal.h>
#include "../include/ConfigParser.hpp"
#include "../include/Server.hpp"

int main(int argc, char **argv)
{
	try
	{
		if (argc != 2)
			throw std::invalid_argument("Usage: ./webserv <config_file>.conf");

		signal(SIGPIPE, SIG_IGN);

		ConfigParser parser(argv[1]);
		std::vector<ServerConfig> servers = parser.parse();
		std::cout << "Parsed " << servers.size() << " server blocks." << std::endl;
		Server manager(servers);
		manager.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
