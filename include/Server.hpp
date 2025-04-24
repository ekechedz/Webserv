#ifndef SERVER_HPP
#define SERVER_HPP

#include "Webserver.hpp"
class Location;

class Server
{
	private:
		struct sockaddr_in servAddr;
		std::vector<Location> locations;
		uint16_t port;
		std::string servName;
		in_addr_t host;
		std::string root;
		unsigned long maxBodySize;
		std::string index;
		std::map<short, std::string> errorPages;
		bool autoIndex;
		std::string redirect;
		int listenFd;

	public:

};

#endif
