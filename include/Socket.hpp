#pragma once

#include <string>
#include <ctime>
#include <iostream>

class ServerConfig;

class Socket
{
public:
	enum Type { LISTENING, CLIENT };
	enum State { RECEIVING, SENDING };

	Socket();
	Socket(int newFD, Type newType, State newState, const ServerConfig* newServer);
	Socket(const Socket& other);
	Socket& operator=(const Socket& other);
	~Socket();

	int getFd() const;
	Type getType() const;
	const ServerConfig& getServerConfig() const;

	void setFD(const int newFD);
	void setValues(const int newFD, const Type newType, const State newState, const ServerConfig* newServer);
	const std::string &getBuffer() const;
	void appendToBuffer(const std::string &data);
	void clearBuffer();

	time_t getLastActivity() const;
	void updateActivity();
	bool hasTimedOut(int timeoutSeconds) const;
	friend std::ostream& operator<<(std::ostream& lhs, const Socket& rhs);

private:
	int _fd;
	std::string _buffer;
	time_t _lastActivity;
	Type _type;
	State _state;
	const ServerConfig* _server;
};

