#include "Logger.hpp"
#include "ScopeLock.hpp"

Logger::Logger(void) {}
Logger::~Logger(void) {}

Logger &Logger::getInstance(void)
{
	static Logger singleton_obj;
	return (singleton_obj);
}

int Logger::init(int fd)
{
	_fd = fd;
	pthread_mutex_init(&_output_mutex, NULL);
	return (0);
}

int Logger::output(std::string msg)
{
	ScopeLock<pthread_mutex_t> lock(&_output_mutex);
	msg += "\n";
	write(_fd, msg.c_str(), msg.length());
	return (0);
}
