#include <iostream>
#include <unistd.h>
#include <string>
#include <sstream>

#include "Client.hpp"
#include "Channel.hpp"
#include "ClientInfo.hpp"
#include "Eventq.hpp"
#include "ScopeLock.hpp"

Client::Client(int fd)
: _fd(fd), _username("*"), _nickname("*"), _realname("*"), \
_passwd_ok(false), _user_state(NEEDREG)
{
	pthread_mutex_init(&_client_input_m, NULL);
	pthread_mutex_init(&_client_output_m, NULL);
	pthread_rwlock_init(&_client_nickname_m, NULL);
	pthread_rwlock_init(&_client_chan_m, NULL);
}

Client::~Client(void)
{
	std::cout << "[Debug]: " << _fd << " Client Destroy" << '\n' << std::endl;
	close(_fd);
	pthread_mutex_destroy(&_client_input_m);
	pthread_mutex_destroy(&_client_output_m);
	pthread_rwlock_destroy(&_client_nickname_m);
	pthread_rwlock_destroy(&_client_chan_m);
}

void Client::id_set(long id)
{
	_id = id;
}

void Client::pass_set(void)
{
	_passwd_ok = true;
}

void Client::nick_set(std::string &nick)
{
	ScopeLock<pthread_rwlock_t> lock(&_client_nickname_m, WriteLock());
	_nickname = nick;
}

void Client::user_init(std::string &user, std::string &host, std::string &real)
{
	_username = user;
	_realname = real;
	_hostname = host;
}

int Client::get_user_state(void)
{
	return (_user_state);
}

int Client::set_user_state(int state)
{
	_user_state = state;
	return (0);
}

bool Client::is_passuser(void)
{
	return (_passwd_ok);
}

long Client::get_id(void)
{
	return (_id);
}

std::string Client::get_nick(void)
{
	ScopeLock<pthread_rwlock_t> lock(&_client_nickname_m, ReadLock());
	return (_nickname);
}

std::string Client::get_user(void)
{
	return (_username);
}

std::string Client::get_host(void)
{
	return (_hostname);
}

std::string Client::get_prifix(void)
{
	std::stringstream ss;

	ScopeLock<pthread_rwlock_t> lock(&_client_nickname_m, ReadLock());
	ss << ":" << _nickname << "!" << _username << "@" << _hostname;
	return (ss.str());
}

void Client::add_output(std::string str)
{
	str += CRLF;
	{
		ScopeLock<pthread_mutex_t> lock(&_client_output_m);
		_output_buf += str;
	}
	Eventq::getInstance().reg_event(_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, reinterpret_cast<void *>(_id));
}

void Client::clear_buffer(void)
{
	{
		ScopeLock<pthread_mutex_t> lock(&_client_input_m);
		_input_buf.clear();
	}
	{
		ScopeLock<pthread_mutex_t> lock(&_client_output_m);
		_output_buf.clear();
	}
}

bool Client::exist_output(void)
{
	bool res;

	{
		ScopeLock<pthread_mutex_t> lock(&_client_output_m);
		res = !_output_buf.empty();
	}
	return (res);
}

int Client::get_fd(void) const
{
	return (_fd);
}

int Client::client_read(void)
{
	char buf[INPUT_BUF_SIZE];
	int ret_recv;

	ScopeLock<pthread_mutex_t> lock(&_client_input_m);
	ret_recv = recv(_fd, buf, INPUT_BUF_SIZE - 1, 0);
	if (ret_recv == 0)
	{
		std::cout << "[Debug]: " << _fd << " disconnected" << '\n' << std::endl;
		return (DISCONNECT);
	}
	if (ret_recv < 0)
	{
		std::cerr << _fd << " Error occuar " << errno << std::endl;
		return (ERROR);
	}
	buf[ret_recv] = 0;
	_input_buf += buf;
	
	std::cout << "[Debug] " << _nickname << " -> Server" << "\n" << buf << std::endl;
	return (ret_recv);
}

int Client::client_write(void)
{
	int output = 0;


	ScopeLock<pthread_mutex_t> lock(&_client_output_m);
	std::cout << "[Debug] Server -> " << _nickname << "\n" << _output_buf << std::endl;
	if ((output = send(_fd, _output_buf.data(), _output_buf.length(), 0)) == -1)
		return (ERROR);
	_output_buf.erase(0, output);
	return (1);
}

void Client::get_input_buffer(std::string &str)
{
	size_t del = 0;

	{
		ScopeLock<pthread_mutex_t> lock(&_client_input_m);
		del = _input_buf.find(CRLF);
		if (del != std::string::npos)
		{
			str = _input_buf.substr(0, del);
			_input_buf.erase(0, del + 2);
		}
		else
			str.clear();
	}
}

int Client::add_chan(const std::string &channame)
{
	ScopeLock<pthread_rwlock_t> lock(&_client_chan_m, WriteLock());
	_chan.insert(channame);
	return (0);
}

int Client::del_chan(const std::string &channame)
{
	ScopeLock<pthread_rwlock_t> lock(&_client_chan_m, WriteLock());
	_chan.erase(channame);
	return (0);
}

int Client::clear_chan(void)
{
	ScopeLock<pthread_rwlock_t> lock(&_client_chan_m, WriteLock());
	_chan.clear();
	return (0);
}

bool Client::include_chan(const std::string &channame)
{
	ScopeLock<pthread_rwlock_t> lock(&_client_chan_m, WriteLock());
	return (_chan.find(channame) != _chan.end());
}
