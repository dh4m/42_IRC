#include "Bot.hpp"
#include <iostream>
#include <sstream>

Bot::Bot(void)
: _core(BOT_NICK)
{
	memset(&_serv_addr, 0, sizeof(_serv_addr));
}

Bot::~Bot(void) {}

int Bot::connent_serv(unsigned int hostip, int port, char *passwd)
{
	_passwd = passwd;
	_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (_socket == -1)
	{
		std::cerr << "socket() Error" << std::endl;
		return (0);
	}
	int opt = true;
	setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	_serv_addr.sin_family = AF_INET;
	_serv_addr.sin_port = htons(port);
	_serv_addr.sin_addr.s_addr= hostip;
	if (connect(_socket, (struct sockaddr *)&_serv_addr, sizeof(_serv_addr)) == -1)
	{
		std::cerr << "connect() Error" << std::endl;
		return (0);
	}
	return (1);
}

int Bot::run(void)
{
	size_t output_idx = 0;
	std::string bot_nick = BOT_NICK;
	std::stringstream pass_msg;
	char input_tmp[INPUT_TMP_SIZE];

	pass_msg << "PASS " << _passwd << CRLF;
	pass_msg << "NICK " << bot_nick << CRLF;
	pass_msg << "USER bot bot 127.0.0.1 :bot" << CRLF;
	std::string pass_str = pass_msg.str();
	output_idx = 0;
	while (output_idx != pass_str.length())
	{
		int send_size;
		if ((send_size = send(_socket, &pass_str.c_str()[output_idx], 
							pass_str.length() - output_idx, 0)) == -1)
			return (0);
		output_idx += send_size;
	}
	while (1)
	{
		int recv_size;
		if ((recv_size = recv(_socket, input_tmp, INPUT_TMP_SIZE - 1, 0)) == -1)
			return (0);
		if (recv_size == 0)
		{
			return (1);
		}
		input_tmp[recv_size] = 0;
		_input_buf += input_tmp;

		if (!_core.cmd_proc(_input_buf, _output_buf))
			continue;

		output_idx = 0;
		while (output_idx != _output_buf.length())
		{
			int send_size;
			if ((send_size = send(_socket, &_output_buf.c_str()[output_idx], 
								_output_buf.length() - output_idx, 0)) == -1)
			{
				return (0);
			}
			output_idx += send_size;
		}
		_output_buf.clear();
	}
	return (1);
}
