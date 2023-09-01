#pragma once

#ifndef CHANNEL_HPP
# define CHANNEL_HPP

#include <set>
#include <map>
#include <vector>
#include <string>
#include <bitset>
#include <pthread.h>

#include "ScopeLock.hpp"
#include "SharedPtr.hpp"
#include "Client.hpp"

enum e_chanmode
{
	INVITEONLY,
	TOPICRESTRICT
};

class Channel
{
public:
	Channel(std::string name);
	~Channel(void);

	const std::string &get_name(void) const;

	int add_user(ClientRef client, const std::string &key);
	int add_operator(ClientRef client);
	int remove_user(ClientRef client);
	int remove_operator(ClientRef client);

	int is_user(ClientRef client);
	int is_operator(ClientRef client);

	int is_inviteonly(void);
	int is_needpasswd(void);

	int passwd_check(std::string passwd);
	int limit_memb_check(void);

	int add_invite(ClientRef user, ClientRef op);
	int del_invite(ClientRef user);
	int is_invite_memb(ClientRef user);
	

	size_t limit_memb(void);
	size_t curr_memb(void);

	int channel_output(const std::string &content, ClientRef talker);
	int channel_output_someone(const std::string &content, ClientRef talker, std::set<ClientRef> &send);
	int kick_user(ClientRef kick, const std::string &msg, ClientRef op);
	int set_topic(std::string topic, ClientRef op);
	int set_limit(size_t limit, ClientRef op);
	int set_passwd(std::string passwd, ClientRef op);
	int mode_set(std::string modestr, std::vector<std::string> &args, ClientRef op);

	std::string get_topic(void);
	std::string get_namelist_reply(void);
	std::string get_chanmode_str(void);
	std::string get_chantime_str(void);
private:
	int _is_user(ClientRef client);
	int _is_operator(ClientRef client);
	int _is_invite(ClientRef client);
	int _channel_output(const std::string &content, ClientRef talker);

	ClientRef _find_member_nick(std::string &nick);

	pthread_mutex_t _channel_m;

	const std::string _name;
	const time_t _create_time;
	std::string _passwd;
	std::string _topic;

	std::set<ClientRef> _member;

	std::set<ClientRef> _operator;

	std::bitset<2> _mode;

	std::set<ClientRef> _invite_list;

	size_t _user_limit;

	Channel	&operator=(const Channel &copy);
	Channel(const Channel &copy);
};

typedef SharedPtr<Channel> ChanRef;

#endif
