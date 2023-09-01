#pragma once

#ifndef LOGGER_HPP
# define LOGGER_HPP

#include <iostream>
#include <unistd.h>
#include <string>
#include <pthread.h>

class Logger
{
public:
	static Logger &getInstance(void);

	int init(int fd);
	int output(std::string msg);
private:
	int _fd;
	pthread_mutex_t _output_mutex;

	Logger(void);
	~Logger(void);
	Logger(const Logger &copy);
	Logger	&operator=(const Logger &copy);
};

#define SERVER_OUTPUT(msg) Logger::getInstance().output(msg)

#ifdef IRC_DEBUG
	#define DEBUG_OUTPUT(msg) Logger::getInstance().output(msg)
#else
	#define DEBUG_OUTPUT(msg) (void)msg
#endif

#endif
