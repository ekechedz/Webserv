#include "./Webserver.hpp"
#include "ServerConfig.hpp"

class Server {
public:
	Server(const ServerConfig &config);
	~Server();

	void run();
private:
	ServerConfig _config;
	int _serverSocket;
	std::vector<struct pollfd> _pollFds; // this is traking the socket that we'll monitoring

	void setupServerSocket();
	void acceptNewConnection();
	void handleRequest(int clientSocket);
	void sendResponse(int clientSocket, const std::string &response);
};
