#pragma once

#ifndef SERVER_HPP
# define SERVER_HPP

# define ACCEPT_EV_NUM 10

#include <iostream>
#include <map>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/event.h>
#include <sys/types.h>

#include "Eventq.hpp"
#include "SharedPtr.hpp"
#include "Channel.hpp"
#include "ClientInfo.hpp"
#include "Worker.hpp"

#define COUNTOF(_x) (sizeof(_x) / sizeof((_x)[0]))

#define SERVER_NAME "ft.irc.local"
#define OPER_NAME "dham"
#define OPER_PASSWD "ircishard"

class Server
{
public:
	Server(int port, const char *passwd);
	~Server(void);

	int init(void);
	int run(void);
private:
	int _port;
	std::string _passwd;
	int _socket;
	struct sockaddr_in _addr;

	Eventq &_ev_q;

	Server	&operator=(const Server &copy);
	Server(const Server &copy);
};

#endif
