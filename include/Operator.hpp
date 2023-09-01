#pragma once

#ifndef OPERATOR_HPP
# define OPERATOR_HPP

#include <vector>
#include <string>
#include <sstream>

#include "Server.hpp"
#include "Channel.hpp"
#include "ClientInfo.hpp"

#define SPACE ' '

enum e_cmd_msg
{
	CMD_PASS, 
	CMD_NICK,
	CMD_USER,
	CMD_JOIN,
	CMD_PRIVMSG,
	CMD_NOTICE,
	CMD_OPER,
	CMD_KICK,
	CMD_INVITE,
	CMD_MODE,
	CMD_TOPIC,
	CMD_QUIT,
	CMD_PART,
	CMD_PING,
	UNKNOWN
};

enum e_reply
{
	ERR_ALREADYRGISTRED = 1,
	ERR_NEEDMOREPARAMS,
	ERR_PASSWDMISMATCH,
	ERR_NICKNAMEINUSE,
	ERR_ERRONEUSNICKNAME,
	ERR_NONICKNAMEGIVEN,
	ERR_INVITEONLYCHAN,
	ERR_BADCHANNELKEY,
	ERR_CHANNELISFULL,
	ERR_NOSUCHCHANNEL,
	ERR_NORECIPIENT,
	ERR_CANNOTSENDTOCHAN,
	ERR_NOSUCHNICK,
	ERR_NOTEXTTOSEND,
	ERR_NOOPERHOST,
	ERR_USERNOTINCHANNEL,
	ERR_NOTONCHANNEL,
	ERR_CHANOPRIVSNEEDED,
	ERR_USERONCHANNEL,
	ERR_UNKNOWNMODE,
	ERR_USERSDONTMATCH,
	ERR_UMODEUNKNOWNFLAG,

	RPL_WELCOME,
	RPL_YOURHOST,
	RPL_TOPIC,
	RPL_NAMREPLY,
	RPL_ENDOFNAMES,
	RPL_YOUREOPER,
	RPL_NOTOPIC,
	RPL_INVITING,
	RPL_CHANNELMODEIS,
	RPL_CREATIONTIME,
	RPL_UMODEIS
};

class Operator
{
public:
	Operator(ClientInfo &info, std::string passwd);
	~Operator(void);

	int cmd_proc(const std::string &cmd_str, ClientRef sender);

	static int reply_send(int reply, std::string param, ClientRef sender);
private:
	int _parsing_msg(void);
	int _prefix_setting(void);
	int _cmd_setting(void);
	void _argu_setting(void);

	int _pass(void);
	int _nick(void);
	int _avail_nick(void);
	int _user(void);
	int _join(void);
	int _privmsg(void);
	int _notice(void);
	int _oper(void);
	int _kick(void);
	int _invite(void);
	int _mode(void);
	int _topic(void);
	int _quit(void);
	int _part(void);
	int _ping(void);

	ClientInfo &_info;
	std::string _cmd_str;
	size_t _cmd_length;
	size_t _cmd_idx;
	ClientRef _sender;

	std::string _prefix;
	int _command;
	std::string _command_str;
	std::vector<std::string> _argu;

	std::string _passwd;

	Operator(const Operator &copy);
	Operator	&operator=(const Operator &copy);
};

#endif
