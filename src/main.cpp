#include <iostream>
#include <stdexcept>
#include <vector>
#include <pthread.h>
#include <signal.h>
#include "../include/ConfigParser.hpp"
#include "../include/Server.hpp"

// there is other way for sure but fore now i could handle it like this
void *runServer(void *arg)
{
	Server *server = (Server *)arg;
	server->run();
	return NULL;
}

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

		std::vector<pthread_t> serverThreads;
		for (size_t i = 0; i < servers.size(); ++i)
		{
			Server *server = new Server(servers[i]);
			pthread_t threadId;
			if (pthread_create(&threadId, NULL, runServer, (void *)server) != 0)
				std::cerr << "Error creating thread for server " << i + 1 << std::endl;
			else
				serverThreads.push_back(threadId);
		}
		for (size_t i = 0; i < serverThreads.size(); ++i)
			pthread_join(serverThreads[i], NULL);
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
