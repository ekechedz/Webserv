#pragma once

#include <string>
#include <ctime>

class Client
{
private:
	int _fd;
	std::string _buffer;
	time_t _lastActivity;

public:
	Client();
	Client(int fd);
	~Client();

	int getFd() const;
	const std::string &getBuffer() const;
	void appendToBuffer(const std::string &data);
	void clearBuffer();

	time_t getLastActivity() const;
	void updateActivity();
	bool hasTimedOut(int timeoutSeconds) const;
};
