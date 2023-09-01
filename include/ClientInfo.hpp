#pragma once

#ifndef CLIENTINFO_HPP
# define CLIENTINFO_HPP

#include <map>
#include <string>
#include <vector>
#include <pthread.h>

#include "SharedPtr.hpp"
#include "Client.hpp"
#include "Channel.hpp"

typedef std::map< std::string, ChanRef > t_chanlist;
typedef std::map< int, ClientRef > t_clientlist;
typedef std::map< std::string, ClientRef > t_nicklist;

class ClientInfo
{
public:
	ClientInfo(void);
	~ClientInfo(void);

	int add_client(int fd);
	int reg_client(ClientRef cl, std::string nick);
	ClientRef find_client(int fd);
	ClientRef find_client(std::string nick);
	void remove_client(int fd, const std::string &msg);
	void close_client(int fd);
	int client_nick_change(int fd, std::string nick);
	int nick_dup_check(std::string nick);

	int join_chan(const std::string &name, const std::string key, ClientRef ref);
	int leave_chan(ChanRef chan, ClientRef ref, const std::string &msg);
	int kick_chan(ChanRef chan, ClientRef kick, std::string &msg, ClientRef op);
	ChanRef find_chan(const std::string &name);

	int send_all_chan(const std::string &msg, ClientRef op);

	int leave_all_channel(ClientRef cl, const std::string &msg);
private:
	t_clientlist _cl_list;
	t_nicklist _cl_nick_list;
	pthread_rwlock_t _cl_list_lock;

	t_chanlist _channel;
	pthread_rwlock_t _ch_list_lock;

	ClientInfo	&operator=(const ClientInfo &copy);
	ClientInfo(const ClientInfo &copy);

	static long _client_id_cnt;
};

#endif
