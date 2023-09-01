#include "Server.hpp"

#include <sys/socket.h>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>

Server::Server(int port, const char *passwd)
: _port(port), _passwd(passwd), _ev_q(Eventq::getInstance())
{
	memset(&_addr, 0, sizeof(_addr));
}

Server::~Server(void)
{
	_ev_q.destroy_eventq();
	close(_socket);
}

int Server::init(void)
{
	if (!_ev_q.init())
	{
		std::cerr << "kqueue() Error" << std::endl;
		return (0);
	}
	_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (_socket == -1)
	{
		std::cerr << "socket() Error" << std::endl;
		return (0);
	}
	int opt = true;
	setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	_addr.sin_family = AF_INET;
	_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	_addr.sin_port = htons(_port);
	if (bind(_socket, (struct sockaddr *)&_addr, sizeof(_addr)) == -1)
	{
		std::cerr << "bind() Error" << std::endl;
		return (0);
	}
	if (listen(_socket, 5) == -1)
	{
		std::cerr << "listen() Error" << std::endl;
		return (0);
	}
	fcntl(_socket, F_SETFL, O_NONBLOCK);
	_ev_q.reg_event(_socket, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
	return (1);
}

int Server::run(void)
{
	Worker worker;
	int new_event;
	t_event ev_list[ACCEPT_EV_NUM];
	int i;
	bool server_run = true;


	std::cout << "Server Start" << std::endl;
	std::cout << "[Debug mode]" << "Debug" << std::endl;
	worker.init(_passwd);
	while (server_run)
	{
		new_event = _ev_q.get_event(ev_list, COUNTOF(ev_list));
		if (new_event == -1)
		{
			std::cerr << "kevent() Error" << std::endl;
			worker.destroy();
			return (0);
		}
		
		for (i = 0; i < new_event; i++)
		{
			if (ev_list[i].filter == EVFILT_USER)
				continue ;
			if (ev_list[i].filter == EVFILT_SIGNAL && ev_list[i].ident == SIGINT)
				server_run = false;
			if (ev_list[i].flags & EV_ERROR)
			{
				if (static_cast<int>(ev_list[i].ident) == _socket)
				{
					std::cerr << "server socket Error\nerror code(errno): " \
								<< strerror(ev_list[i].data) << std::endl;
					worker.destroy();
					return (0);
				}
				else if (ev_list[i].data != 2) // no such file or directory의 경우 고려하지 않음, reg_event의 딜레이 때문에 발생
				{
					std::cerr << ev_list[i].ident << " client socket Error\nerror code(errno): " \
								<< strerror(ev_list[i].data) << std::endl;
					worker.remove_client(ev_list[i].ident, "");
					worker.close_client(ev_list[i].ident);
				}
			}
			else if (static_cast<int>(ev_list[i].filter) == EVFILT_READ)
			{
				if (static_cast<int>(ev_list[i].ident) == _socket)
				{
					int cl_fd;
					
					cl_fd = accept(_socket, NULL, NULL);
					if (cl_fd == -1)
					{
						std::cerr << "accept error" << std::endl;
						continue;
					}
					worker.add_client(cl_fd);
				}
				else
				{
					std::cout << "[Debug]: read event to " << ev_list[i].ident << '\n' << std::endl;
					worker.reg_msg(ev_list[i].ident, ev_list[i].filter, reinterpret_cast<long>(ev_list[i].udata));
				}
			}
			else if (static_cast<int>(ev_list[i].filter) == EVFILT_WRITE)
			{
				std::cout << "[Debug]: write event to " << ev_list[i].ident << '\n' << std::endl;
				worker.reg_msg(ev_list[i].ident, ev_list[i].filter, reinterpret_cast<long>(ev_list[i].udata));
			}
		}
	}
	worker.destroy();
	return (1);
}
