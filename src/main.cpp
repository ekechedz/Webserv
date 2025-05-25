#include <iostream>
#include <stdexcept>
#include <vector>
#include <pthread.h>
#include <signal.h>
#include <cstdlib>
#include "../include/ConfigParser.hpp"
#include "../include/Server.hpp"
#include "../include/ServerConfig.hpp"
#include "../include/Logger.hpp"

void handleSigint(int signal)
{
	if (signal == SIGINT)
	{
		logInfo("SIGINT received. Shutting down the server...");
		std::exit(0);
	}
}

int main(int argc, char **argv)
{
	try
	{
		// Setup logger
		Logger::getInstance().setLevel(Logger::INFO);
		Logger::getInstance().setLogFile("webserv.log");
		Logger::getInstance().log(Logger::INFO, "Server starting up");

		if (argc != 2)
		{
			Logger::getInstance().log(Logger::ERROR, "Invalid arguments. Usage: ./webserv <config_file>.conf");
			throw std::invalid_argument("Usage: ./webserv <config_file>.conf");
		}
		
		// Register the SIGINT handler
		signal(SIGINT, handleSigint);
		logInfo("SIGINT handler registered. Press Ctrl+C to stop the server.");

		signal(SIGPIPE, SIG_IGN);

		ConfigParser parser(argv[1]);
		std::vector<ServerConfig> servers = parser.parse();
		Server manager(servers);
		Logger::getInstance().log(Logger::INFO, "Server configuration loaded successfully");
		manager.run();
	}
	catch (const std::exception &e)
	{
		Logger::getInstance().log(Logger::CRITICAL, std::string("Fatal error: ") + e.what());
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
