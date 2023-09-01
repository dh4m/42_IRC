#pragma once

#ifndef CLIENT_HPP
# define CLIENT_HPP

#include <string>
#include <vector>
#include <set>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "SharedPtr.hpp"

#define INPUT_BUF_SIZE 1024
#define CRLF "\r\n"

enum e_result
{
	LOCK_FAIL = -2,
	ERROR = -1,
	DISCONNECT = 0,
	SUCCESS = 1
};

enum e_userstate
{
	NEEDREG,
	NEEDNICK,
	AVAIL_USER,
	OPERATOR,
	UNAVAIL_USER,
	CLOSE_USER
};

class Client
{
public:
	Client(int fd);
	~Client(void);

	void id_set(long id);
	void pass_set(void);
	void nick_set(std::string &nick);
	bool is_passuser(void);
	long get_id(void);
	std::string get_nick(void);
	std::string get_user(void);
	std::string get_host(void);
	std::string get_prifix(void);
	void user_init(std::string &user, std::string &host, std::string &real);
	int get_user_state(void);
	int set_user_state(int state);

	void add_output(std::string str);
	void clear_buffer(void);
	int get_fd(void) const;
	void get_input_buffer(std::string &str);
	int client_read(void);
	int client_write(void);
	bool exist_output(void);

	int add_chan(const std::string &channame);
	int del_chan(const std::string &channame);
	int clear_chan(void);
	bool include_chan(const std::string &channame);

private:
	int _fd;
	long _id;
	std::string _username;
	std::string _nickname;
	pthread_rwlock_t _client_nickname_m;
	std::string _hostname;
	std::string _realname;
	bool _passwd_ok;

	int _user_state;

	std::string _input_buf;
	pthread_mutex_t _client_input_m;

	std::string _output_buf;
	pthread_mutex_t _client_output_m;

	std::set<std::string> _chan;
	pthread_rwlock_t _client_chan_m;

	Client	&operator=(const Client &copy);
	Client(const Client &copy);
};

typedef SharedPtr<Client> ClientRef;

#endif
