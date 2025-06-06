#include "../include/Socket.hpp"

Socket::Socket()
: _fd(-1)
, _lastActivity(std::time(NULL))
, _type(LISTENING)
, _state(RECEIVING)
, _nbrRequests(0)
{}

Socket::Socket(int newFD, Type newType, State newState, const std::string IPv4, const int port)
: _fd(newFD)
, _lastActivity(std::time(NULL))
, _type(newType)
, _state(newState)
, _nbrRequests(0)
, _IPv4(IPv4)
, _port(port)
{}

Socket::Socket(const Socket& other)
: _fd(other._fd)
, _buffer(other._buffer)
, _lastActivity(other._lastActivity)
, _type(other._type)
, _state(other._state)
, _nbrRequests(other._nbrRequests)
, _IPv4(other._IPv4)
, _port(other._port)
{}

Socket& Socket::operator=(const Socket& other)
{
	if (this != &other)
	{
		_fd = other._fd;
		_buffer = other._buffer;
		_lastActivity = other._lastActivity;
		_type = other._type;
		_state = other._state;
		_nbrRequests = other._nbrRequests;
		_IPv4 = other._IPv4;
		_port = other._port;
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


const std::string &Socket::getBuffer() const
{
	return _buffer;
}

std::string Socket::getIPv4() const
{
	return _IPv4;
}
int Socket::getPort() const
{
	return _port;
}
bool Socket::getNeedsToClose() const
{
	return _needsToClose;
}
Socket::State Socket::getState() const
{
	return _state;
}



void Socket::appendToBuffer(const char* data, size_t len) {
    _buffer.append(data, len);
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
void Socket::setState(State newState)
{
	_state = newState;
}
void Socket::setNeedsToClose(bool needsToClose)
{
	_needsToClose = needsToClose;
}
void Socket::trimBuffer(size_t len)
{
	if (len < _buffer.size())
	{
		_buffer.erase(0, len);
	}
	else
	{
		clearBuffer();
	}
}


std::ostream& operator<<(std::ostream& lhs, const Socket& rhs)
{
	lhs << "Socket FD: " << rhs._fd
		<< ", Type: " << (rhs._type == Socket::LISTENING ? "LISTENING" : "CLIENT")
		<< ", State: " << (rhs._state == Socket::RECEIVING ? "RECEIVING" : "SENDING")
		<< ", Buffer Size: " << rhs._buffer.size()
		<< ", IPv4: " << rhs._IPv4
		<< ", Port: " << rhs._port
		<< ", Nbr Requests: " << rhs._nbrRequests
		<< ", Last Activity: " << std::ctime(&rhs._lastActivity);
	return lhs;
}

void Socket::setValues(const int newFD, const Type newType, const State newState)
{
	_fd = newFD;
	_type = newType;
	_state = newState;
}

int Socket::getNbrRequests() const
{
	return _nbrRequests;
}

void Socket::increaseNbrRequests()
{
	++_nbrRequests;
}
