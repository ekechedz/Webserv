#include "../include/ConfigParser.hpp"
#include "../include/Utils.hpp"
#include <csignal>
#include <iostream>

int main(int argc, char **argv)
{
	try {
		if (argc != 2)
			throw std::invalid_argument("Usage: ./webserv <config_file>.conf");

		signal(SIGPIPE, SIG_IGN);

		ConfigParser parser(argv[1]);
		std::vector<ServerConfig> servers = parser.parse();

		for (size_t i = 0; i < servers.size(); ++i)
			servers[i].print();

	} catch (const std::exception &e) {
		printError(e.what());
		return 1;
	}
	return 0;
}
