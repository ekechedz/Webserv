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
	Socket(int newFD, Type newType, State newState, const std::string IPv4, const int port);
	Socket(const Socket& other);
	Socket& operator=(const Socket& other);
	~Socket();

	// Getters
	int getFd() const;
	Type getType() const;
	time_t getLastActivity() const;
	const std::string &getBuffer() const;
	int getNbrRequests() const;
	std::string getIPv4() const;
	int getPort() const;

	void increaseNbrRequests();
	void setFD(const int newFD);
	void setValues(const int newFD, const Type newType, const State newState);
	void appendToBuffer(const char* data, size_t len);
	void clearBuffer();
	void setState(State newState);

	void updateActivity();
	bool hasTimedOut(int timeoutSeconds) const;
	friend std::ostream& operator<<(std::ostream& lhs, const Socket& rhs);

private:
	int _fd;
	std::string _buffer;
	time_t _lastActivity;
	Type _type;
	State _state;
	int _nbrRequests;
	std::string _IPv4;
	int _port;
};
