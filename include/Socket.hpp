#pragma once

#include <string>
#include <ctime>

class ServerConfig;

class Socket
{
private:
	int _fd;
	std::string _buffer;
	time_t _lastActivity;

public:
	enum Type { LISTENING, CLIENT } type;
	enum State { RECEIVING, SENDING } state;
	const ServerConfig* server;

	Socket();
	Socket(int fd);
	~Socket();
	
	int getFd() const;
	const std::string &getBuffer() const;
	void appendToBuffer(const std::string &data);
	void clearBuffer();

	time_t getLastActivity() const;
	void updateActivity();
	bool hasTimedOut(int timeoutSeconds) const;
};
