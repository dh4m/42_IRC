#include "BotCore.hpp"
#include <ctime>
#include <iostream>

BotCore::BotCore(std::string bot_nick)
: _bot_nick(bot_nick)
{}

BotCore::~BotCore(void) {}

int BotCore::cmd_proc(std::string &input, std::string &output)
{
	std::string input_str;
	size_t start_idx = 0;
	size_t idx = 0;

	idx = input.find(CRLF, start_idx);
	while (idx != std::string::npos)
	{
		_one_cmd_proc(input.substr(start_idx, idx - start_idx));
		start_idx = idx + 2;
		idx = input.find(CRLF, start_idx);
	}
	input.erase(0, start_idx);
	if (_output_buf.empty())
		return (0);
	output = _output_buf;
	_output_buf.clear();
	std::cout << "[Debug]: output:\n" << output << std::endl;
	return (1);
}

int BotCore::_one_cmd_proc(const std::string &input)
{
	_input_str = input;
	std::cout << "[Debug]: input command:\n" << _input_str << std::endl;
	if (!_parsing_msg())
		return (0);
	switch (_cmd)
	{
	case INVITE:
		_invite();
		break;
	case JOIN:
		_join();
		break;
	case PRIVMSG:
		_privmsg();
		break;
	case NICKDUP_REPLY:
		_re_nick();
		break;
	default: // NONE
		break;
	}
	return (1);
}

int BotCore::_parsing_msg(void)
{
	_input_length = _input_str.length();
	_input_idx = 0;
	_argu.clear();
	if (_input_str[0] == ':')
	{
		if (!_prefix_setting())
			return (0);
	}
	if (!_cmd_setting())
		return (0);
	_argu_setting();
	return (1);
}

int BotCore::_prefix_setting(void)
{
	_input_idx = _input_str.find(SPACE);
	if (_input_idx == std::string::npos || _input_idx == 1)
		return (0); // unavailable prefix
	_prefix_str = _input_str.substr(1, _input_idx - 1);
	while (_input_idx < _input_length && _input_str[_input_idx] == SPACE)
		_input_idx++;
	return (1);
}

int BotCore::_cmd_setting(void)
{
	size_t cmd_len = _input_str.find(SPACE, _input_idx) - _input_idx;
	_cmd_str = _input_str.substr(_input_idx, cmd_len);

	if (_cmd_str == "INVITE")
		_cmd = INVITE;
	else if (_cmd_str == "JOIN")
		_cmd = JOIN;
	else if (_cmd_str == "PRIVMSG")
		_cmd = PRIVMSG;
	else if (_cmd_str == "433")
		_cmd = NICKDUP_REPLY;
	else
		_cmd = NONE;

	_input_idx = _input_idx + cmd_len;
	if (_input_idx != std::string::npos)
	{
		while (_input_idx < _input_length && _input_str[_input_idx] == SPACE)
			_input_idx++;
	}
	return (_cmd);
}

int BotCore::_argu_setting(void)
{
	while (_input_idx != std::string::npos)
	{
		if (_input_str[_input_idx] == ':')
		{
			std::string cmd = _input_str.substr(_input_idx + 1);
			_argu.push_back(cmd);
			_input_idx = std::string::npos;
		}
		else
		{
			size_t cmd_len = _input_str.find(SPACE, _input_idx) - _input_idx;
			std::string cmd = _input_str.substr(_input_idx, cmd_len);
			_argu.push_back(cmd);
			_input_idx = _input_idx + cmd_len;
			if (_input_idx != std::string::npos)
			{
				while (_input_idx < _input_length && _input_str[_input_idx] == SPACE)
					_input_idx++;
			}
		}
	}
	return (1);
}

int BotCore::_privmsg(void)
{
	if (_argu.size() < 2)
		return (0);
	if (_argu[1][0] != '!')
		return (1);
	std::stringstream respon_msg;
	time_t now = time(0);
	char *now_time = ctime(&now);
	now_time[strlen(now_time) - 1] = 0;

	respon_msg << "NOTICE " << _argu[0] << " :";
	if (_argu[1] == "!time")
		respon_msg << "Let me tell you the current time! The current time is "
					<< now_time << "!" << CRLF;
	else if (_argu[1] == "!name")
		respon_msg << "My name is IrcBot! Not satisfied? Please tell the person who made it." << CRLF;
	else if (_argu[1] == "!developer")
		respon_msg << "The person who made me is dham!" << CRLF;
	else if (_argu[1] == "!donate")
		respon_msg << "Buy a cup of coffee for the developer: "
					<< "kukmin 943202-00-712286" << CRLF;
	else
		respon_msg << "I'm not sure what you're saying.. The developer made it sloppily.." << CRLF;
	_output_buf += respon_msg.str();
	return (1);
}

int BotCore::_invite(void)
{
	if (_argu.size() < 2)
		return (0);
	std::stringstream respon_msg;

	respon_msg << "JOIN :" << _argu[1] << CRLF;
	_output_buf += respon_msg.str();
	return (1);
}

int BotCore::_join(void)
{
	if (_argu.size() < 1)
		return (0);
	std::stringstream respon_msg;
	std::string join_nick;

	join_nick = _prefix_str.substr(0, _prefix_str.find('!'));
	if (join_nick == _bot_nick)
		respon_msg << "NOTICE " << _argu[0] << " :Hello everyone!! my name is " << _bot_nick << "! (usage: !<command>)" << CRLF;
	else
		respon_msg << "NOTICE " << _argu[0] << " :" << join_nick << "! Welcome to " << _argu[0] << "!!" << CRLF;
	_output_buf += respon_msg.str();
	return (1);
}

int BotCore::_re_nick(void)
{
	std::stringstream respon_msg;

	_bot_nick += '_';
	respon_msg << "NICK " << _bot_nick << CRLF;
	_output_buf += respon_msg.str();
	return (1);
}