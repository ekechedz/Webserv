#include "../include/Socket.hpp"

Socket::Socket() : _fd(-1), _lastActivity(std::time(NULL)) {}

Socket::Socket(int fd) : _fd(fd), _lastActivity(std::time(NULL)) {}

Socket::~Socket() {}

int Socket::getFd() const
{
	return _fd;
}

const std::string &Socket::getBuffer() const
{
	return _buffer;
}

void Socket::appendToBuffer(const std::string &data)
{
	_buffer += data;
	updateActivity();
}

void Socket::clearBuffer()
{
	_buffer.clear();
}

time_t Socket::getLastActivity() const
{
	return _lastActivity;
}

void Socket::updateActivity()
{
	_lastActivity = std::time(NULL);
}

bool Socket::hasTimedOut(int timeoutSeconds) const
{
	return std::difftime(std::time(NULL), _lastActivity) > timeoutSeconds;
}
