#pragma once

#ifndef BOT_HPP
# define BOT_HPP

#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/event.h>
#include <sys/types.h>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>

#include "BotCore.hpp"

#define CRLF "\r\n"
#define BOT_NICK "irc_Bot"
#define INPUT_TMP_SIZE 1024

class Bot
{
public:
	Bot(void);
	~Bot(void);

	int connent_serv(unsigned int hostip, int port, char *passwd);
	int run(void);
private:
	BotCore _core;

	struct sockaddr_in _serv_addr;
	std::string _passwd;

	int _socket;

	std::string _input_buf;
	std::string _output_buf;

	Bot(const Bot &copy);
	Bot	&operator=(const Bot &copy);
};

#endif
