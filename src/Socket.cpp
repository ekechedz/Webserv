#include "../include/Socket.hpp"

Socket::Socket()
: _fd(-1)
, _lastActivity(std::time(NULL))
, _type(LISTENING)
, _state(RECEIVING)
, _server(NULL)
, _nbrRequests(0)
{}

Socket::Socket(int newFD, Type newType, State newState, const ServerConfig* newServer)
: _fd(newFD)
, _lastActivity(std::time(NULL))
, _type(newType)
, _state(newState)
, _server(newServer)
, _nbrRequests(0)
{}

Socket::Socket(const Socket& other)
: _fd(other._fd)
, _lastActivity(other._lastActivity)
, _type(other._type)
, _state(other._state)
, _server(other._server)
, _nbrRequests(other._nbrRequests)
{}

Socket& Socket::operator=(const Socket& other)
{
	if (this != &other)
	{
		_fd = other._fd;
		_lastActivity = other._lastActivity;
		_type = other._type;
		_state = other._state;
		_server = other._server;
		_nbrRequests = other._nbrRequests;
	}
	return *this;
}

Socket::~Socket() {}

int Socket::getFd() const
{
	return _fd;
}

Socket::Type Socket::getType() const
{
	return _type;
}

const ServerConfig& Socket::getServerConfig() const
{
	return *_server;
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
void Socket::setFD(const int newFD)
{
	_fd = newFD;
}

std::ostream& operator<<(std::ostream& lhs, const Socket& rhs)
{
	lhs << "Socket FD: " << rhs._fd
		<< ", Type: " << (rhs._type == Socket::LISTENING ? "LISTENING" : "CLIENT")
		<< ", State: " << (rhs._state == Socket::RECEIVING ? "RECEIVING" : "SENDING")
		<< ", Buffer Size: " << rhs._buffer.size()
		<< ", Last Activity: " << std::ctime(&rhs._lastActivity); 
	return lhs;
}

void Socket::setValues(const int newFD, const Type newType, const State newState, const ServerConfig* newServer)
{
	_fd = newFD;
	_type = newType;
	_state = newState;
	_server = newServer;
}

int Socket::getNbrRequests() const
{
	return _nbrRequests;
}

void Socket::increaseNbrRequests()
{
	++_nbrRequests;
}