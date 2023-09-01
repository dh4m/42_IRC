#pragma once

#ifndef BOTCORE_HPP
# define BOTCORE_HPP

#include <string>
#include <sstream>
#include <vector>

#define CRLF "\r\n"
#define SPACE ' '

enum e_cmd
{
	NONE,
	INVITE,
	JOIN,
	PRIVMSG,
	NICKDUP_REPLY
};

class BotCore
{
public:
	BotCore(std::string bot_nick);
	~BotCore(void);

	int cmd_proc(std::string &input, std::string &output);
private:
	std::string _bot_nick;

	int _one_cmd_proc(const std::string &input);

	int _parsing_msg(void);
	int _prefix_setting(void);
	int _cmd_setting(void);
	int _argu_setting(void);

	int _privmsg(void);
	int _invite(void);
	int _join(void);
	int _re_nick(void);

	std::string _input_str;
	size_t _input_length;
	size_t _input_idx;

	int _cmd;
	std::string _prefix_str;
	std::string _cmd_str;
	std::vector<std::string> _argu;

	std::string _output_buf;

	BotCore(const BotCore &copy);
	BotCore	&operator=(const BotCore &copy);
};

#endif
