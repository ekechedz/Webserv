#include "../include/Client.hpp"

Client::Client() : _fd(-1), _lastActivity(std::time(NULL)) {}

Client::Client(int fd) : _fd(fd), _lastActivity(std::time(NULL)) {}

Client::~Client() {}

int Client::getFd() const
{
	return _fd;
}

const std::string &Client::getBuffer() const
{
	return _buffer;
}

void Client::appendToBuffer(const std::string &data)
{
	_buffer += data;
	updateActivity();
}

void Client::clearBuffer()
{
	_buffer.clear();
}

time_t Client::getLastActivity() const
{
	return _lastActivity;
}

void Client::updateActivity()
{
	_lastActivity = std::time(NULL);
}

bool Client::hasTimedOut(int timeoutSeconds) const
{
	return std::difftime(std::time(NULL), _lastActivity) > timeoutSeconds;
}
