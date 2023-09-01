#include "Operator.hpp"

Operator::Operator(ClientInfo &info, std::string passwd)
: _info(info), _passwd(passwd)
{}

Operator::~Operator(void)
{}

int Operator::cmd_proc(const std::string &cmd_str, ClientRef sender)
{
	int user_state = sender->get_user_state();
	if (user_state == UNAVAIL_USER || user_state == CLOSE_USER)
		return (0);
	int res = 0;

	_cmd_idx = 0;
	_cmd_str = cmd_str;
	_cmd_length = cmd_str.length();
	_sender = sender;
	_argu.clear();

	std::cout << "[Debug]: operating str\n" << cmd_str << '\n' << std::endl;
	if (!_parsing_msg())
	{
		_sender = ClientRef::NullSharedPtr();
		return (1); // unknown cmd
	}
	if (_command != CMD_PASS && _command != UNKNOWN
		&& !_sender->is_passuser())
	{
		_info.remove_client(_sender->get_fd(), "");
		reply_send(ERR_PASSWDMISMATCH, "", _sender); // ERR_PASSWDMISMATCH
		_sender = ClientRef::NullSharedPtr();
		return (0);
	}
	if (_command != CMD_PASS && _command != CMD_NICK && _command != CMD_USER && _command != UNKNOWN \
		&& !(_sender->get_user_state() == AVAIL_USER || _sender->get_user_state() == OPERATOR))
	{
		_sender = ClientRef::NullSharedPtr();
		return (0);
	}
	switch (_command)
	{
	case CMD_PASS:
		res = _pass();
		break;
	case CMD_NICK:
		res = _nick();
		break;
	case CMD_USER:
		res = _user();
		break;
	case CMD_JOIN:
		res = _join();
		break;
	case CMD_PRIVMSG:
		res = _privmsg();
		break;
	case CMD_NOTICE:
		res = _notice();
		break;
	case CMD_OPER:
		res = _oper();
		break;
	case CMD_KICK:
		res = _kick();
		break;
	case CMD_INVITE:
		res = _invite();
		break;
	case CMD_MODE:
		res = _mode();
		break;
	case CMD_TOPIC:
		res = _topic();
		break;
	case CMD_QUIT:
		res = _quit();
		break;
	case CMD_PART:
		res = _part();
		break;
	case CMD_PING:
		res = _ping();
		break;
	case UNKNOWN:
		break;
	default:
		break;
	}
	(void) res;
	_sender = ClientRef::NullSharedPtr();
	return (1);
}

int Operator::_parsing_msg(void)
{
	if (_cmd_str[0] == ':')
	{
		if (!_prefix_setting()) // prefix
			return (0); // unavailable prefix
	}
	if (_cmd_setting() == UNKNOWN) // command setting
		return (0); // unknown cmd
	_argu_setting();
	return (1);
}

int Operator::_prefix_setting(void)
{
	_cmd_idx = _cmd_str.find(SPACE);
	if (_cmd_idx == std::string::npos || _cmd_idx == 1)
		return (0); // unavailable prefix
	_prefix = _cmd_str.substr(1, _cmd_idx - 1);
	while (_cmd_idx < _cmd_length && _cmd_str[_cmd_idx] == SPACE)
		_cmd_idx++;
	return (1);
}

int Operator::_cmd_setting(void)
{
	size_t cmd_len = _cmd_str.find(SPACE, _cmd_idx) - _cmd_idx;
	_command_str = _cmd_str.substr(_cmd_idx, cmd_len);

	if (_command_str == "PASS")
		_command = CMD_PASS;
	else if (_command_str == "NICK")
		_command = CMD_NICK;
	else if (_command_str == "USER")
		_command = CMD_USER;
	else if (_command_str == "JOIN")
		_command = CMD_JOIN;
	else if (_command_str == "PRIVMSG")
		_command = CMD_PRIVMSG;
	else if (_command_str == "NOTICE")
		_command = CMD_NOTICE;
	else if (_command_str == "OPER")
		_command = CMD_OPER;
	else if (_command_str == "KICK")
		_command = CMD_KICK;
	else if (_command_str == "INVITE")
		_command = CMD_INVITE;
	else if (_command_str == "MODE")
		_command = CMD_MODE;
	else if (_command_str == "TOPIC")
		_command = CMD_TOPIC;
	else if (_command_str == "QUIT")
		_command = CMD_QUIT;
	else if (_command_str == "PART")
		_command = CMD_PART;
	else if (_command_str == "PING")
		_command = CMD_PING;
	else
		_command = UNKNOWN;

	_cmd_idx = _cmd_idx + cmd_len;
	if (_cmd_idx != std::string::npos)
	{
		while (_cmd_idx < _cmd_length && _cmd_str[_cmd_idx] == SPACE)
			_cmd_idx++;
	}
	return (_command);
}

void Operator::_argu_setting(void)
{
	while (_cmd_idx != std::string::npos)
	{
		if (_cmd_str[_cmd_idx] == ':')
		{
			std::string cmd = _cmd_str.substr(_cmd_idx + 1);
			_argu.push_back(cmd);
			_cmd_idx = std::string::npos;
		}
		else
		{
			size_t cmd_len = _cmd_str.find(SPACE, _cmd_idx) - _cmd_idx;
			std::string cmd = _cmd_str.substr(_cmd_idx, cmd_len);
			_argu.push_back(cmd);
			_cmd_idx = _cmd_idx + cmd_len;
			if (_cmd_idx != std::string::npos)
			{
				while (_cmd_idx < _cmd_length && _cmd_str[_cmd_idx] == SPACE)
					_cmd_idx++;
			}
		}
	}
}

int Operator::_pass(void)
{
	if ((*_sender).get_user_state() != NEEDREG)
	{
		reply_send(ERR_ALREADYRGISTRED, "", _sender); // ERR_ALREADYRGISTRED
		return (0);
	}
	if (_argu.empty())
	{
		reply_send(ERR_NEEDMOREPARAMS, _command_str, _sender); // ERR_NEEDMOREPARAMS
		return (0);
	}
	if (_argu[0] != _passwd)
	{
		_info.remove_client(_sender->get_fd(), "");
		reply_send(ERR_PASSWDMISMATCH, "", _sender); // ERR_PASSWDMISMATCH
		return (0);
	}
	
	_sender->pass_set();
	return (1);
}

int Operator::_avail_nick(void)
{
	std::string nick = _argu[0];
	size_t nick_len = nick.length();

	if (!isalpha(nick[0]))
		return (0);
	for (size_t i = 0; i < nick_len; i++)
	{
		if (!isalpha(nick[i]) && !isdigit(nick[i]) && nick[i] != '-' \
			&& nick[i] != '[' && nick[i] != ']' && nick[i] != '\\' && nick[i] != '`' \
			&& nick[i] != '^' && nick[i] != '{' && nick[i] != '}' && nick[i] != '_')
			return (0);
	}
	return (1);
}

int Operator::_nick(void)
{
	if (_argu.size() == 0)
	{
		reply_send(ERR_NONICKNAMEGIVEN, "", _sender);
		return (0);
	}
	if (_sender->get_user_state() == NEEDREG) //첫 등록
	{
		if (!_avail_nick())
		{
			_info.remove_client(_sender->get_fd(), "");
			reply_send(ERR_ERRONEUSNICKNAME, _argu[0], _sender);
			return (0);
		}
		if (_info.nick_dup_check(_argu[0]))
		{
			reply_send(ERR_NICKNAMEINUSE, _argu[0], _sender);
			_sender->set_user_state(NEEDNICK);
			return (0);
		}
		_sender->nick_set(_argu[0]);
	}
	else if (_sender->get_user_state() == NEEDNICK) // 등록시 닉네임 중복된경우
	{
		if (!_avail_nick())
		{
			reply_send(ERR_ERRONEUSNICKNAME, _argu[0], _sender);
			return (0);
		}
		if (_info.nick_dup_check(_argu[0]))
		{
			reply_send(ERR_NICKNAMEINUSE, _argu[0], _sender);
			return (0);
		}
		_sender->nick_set(_argu[0]);
		if (_sender->get_user() != "*") // user커맨드를 거친 경우
		{
			_info.reg_client(_sender, _sender->get_nick());
			std::stringstream nickmsg;
			nickmsg << _sender->get_prifix() << " NICK " << _argu[0];
			_sender->add_output(nickmsg.str());
			reply_send(RPL_WELCOME, "", _sender);
			reply_send(RPL_YOURHOST, "", _sender);
		}
	}
	else // 닉네임 변경
	{
		if (!_avail_nick())
		{
			reply_send(ERR_ERRONEUSNICKNAME, _argu[0], _sender);
			return (0);
		}
		if (_info.nick_dup_check(_argu[0]))
		{
			reply_send(ERR_NICKNAMEINUSE, _argu[0], _sender);
			return (0);
		}
		_info.client_nick_change(_sender->get_fd(), _argu[0]);
	}
	return (1);
}

int Operator::_user(void)
{
	if (_sender->get_user() != "*")
	{
		reply_send(ERR_ALREADYRGISTRED, "", _sender); // ERR_ALREADYRGISTRED
		return (0);
	}
	if (_argu.size() < 4)
	{
		reply_send(ERR_NEEDMOREPARAMS, _command_str, _sender); // ERR_NEEDMOREPARAMS
		return (0);
	}
	_sender->user_init(_argu[0], _argu[1], _argu[3]);
	if (_sender->get_nick() == "*")
		_sender->set_user_state(NEEDNICK);
	else if (_sender->get_user_state() != NEEDNICK)
	{
		_info.reg_client(_sender, _sender->get_nick());
		reply_send(RPL_WELCOME, "", _sender);
		reply_send(RPL_YOURHOST, "", _sender);
	}
	return (1);
}

int Operator::_join(void)
{
	if (_argu.size() == 0)
	{
		reply_send(ERR_NEEDMOREPARAMS, _command_str, _sender);
		return (0);
	}

	int res;
	std::stringstream chan_list(_argu[0]);
	std::stringstream key_list;
	if (_argu.size() == 2)
		key_list << _argu[1];
	else
		key_list.clear();
	std::string chan, key;
	std::getline(chan_list, chan, ',');
	std::getline(key_list, key, ',');
	while (chan_list)
	{
		res = _info.join_chan(chan, key, _sender);
		if (res > 0) // join 에러가 난 경우, join_chan함수 안에서 에러 처리
		{
			reply_send(res, chan, _sender); // join실패
		}
		else if (res == 0) // join 성공한 경우
		{
			std::stringstream join_msg;
			join_msg << _sender->get_prifix() << " JOIN :" << chan;
			ChanRef join_chan = _info.find_chan(chan);
			if (!join_chan)
				return (0);
			std::string topic = join_chan->get_topic();
			_sender->add_output(join_msg.str());
			if (!topic.empty())
				reply_send(RPL_TOPIC, topic, _sender);
			reply_send(RPL_NAMREPLY, join_chan->get_namelist_reply(), _sender);
			reply_send(RPL_ENDOFNAMES, chan, _sender);
			join_chan->channel_output(join_msg.str(), _sender);
		}
		std::getline(chan_list, chan, ',');
		if (key_list)
			std::getline(key_list, key, ',');
		else
			key = "";
	}
	return (1);
}

int Operator::_privmsg(void)
{
	if (_argu.size() < 2)
	{
		if (_argu.size() == 0 || _argu[0][0] == ':')
			reply_send(ERR_NORECIPIENT, _command_str, _sender);
		else
			reply_send(ERR_NOTEXTTOSEND, "", _sender);
		return (0);
	}
	std::stringstream send_msgstream;
	//prefix
	send_msgstream << _sender->get_prifix() << " ";
	//cmd
	send_msgstream << "PRIVMSG" << " ";
	std::string send_msg = send_msgstream.str();
	
	std::stringstream recv_list(_argu[0]);
	std::string recv;
	std::string full_msg;
	std::getline(recv_list, recv, ',');
	while (recv_list)
	{
		full_msg = send_msg + recv + " :" + _argu[1];
		if (recv[0] == '#') // send channel
		{
			ChanRef recv_chan = _info.find_chan(recv);
			if (recv_chan)
			{
				int res = recv_chan->channel_output(full_msg, _sender);
				if (res == ERR_CANNOTSENDTOCHAN)
					reply_send(ERR_CANNOTSENDTOCHAN, recv, _sender);
			}
			else
				reply_send(ERR_CANNOTSENDTOCHAN, recv, _sender);
		}
		else // send user
		{
			ClientRef recv_cl = _info.find_client(recv);
			if (!recv_cl)
				reply_send(ERR_NOSUCHNICK, recv, _sender);// 유저 없음
			else
				recv_cl->add_output(full_msg);
		}
		std::getline(recv_list, recv, ',');
	}
	return (1);
}

int Operator::_notice(void)
{
	if (_argu.size() < 2)
	{
		if (_argu.size() == 0 || _argu[0][0] == ':')
			reply_send(ERR_NORECIPIENT, _command_str, _sender);
		else
			reply_send(ERR_NOTEXTTOSEND, "", _sender);
		return (0);
	}
	std::stringstream send_msgstream;
	//prefix
	send_msgstream << _sender->get_prifix() << " ";
	//cmd
	send_msgstream << "NOTICE" << " ";
	std::string send_msg = send_msgstream.str();
	
	std::stringstream recv_list(_argu[0]);
	std::string recv;
	std::string full_msg;
	std::getline(recv_list, recv, ',');
	while (recv_list)
	{
		full_msg = send_msg + recv + " :" + _argu[1];
		if (recv[0] == '#') // send channel
		{
			ChanRef recv_chan = _info.find_chan(recv);
			if (recv_chan)
			{
				int res = recv_chan->channel_output(full_msg, _sender);
				if (res == ERR_CANNOTSENDTOCHAN)
					reply_send(ERR_CANNOTSENDTOCHAN, recv, _sender);
			}
			else
				reply_send(ERR_CANNOTSENDTOCHAN, recv, _sender);
		}
		else // send user
		{
			ClientRef recv_cl = _info.find_client(recv);
			if (!recv_cl)
				reply_send(ERR_NOSUCHNICK, recv, _sender);// 유저 없음
			else
				recv_cl->add_output(full_msg);
		}
		std::getline(recv_list, recv, ',');
	}
	return (1);
}

int Operator::_oper(void)
{
	if (_argu.size() < 2)
	{
		reply_send(ERR_NEEDMOREPARAMS, _command_str, _sender);
		return (0);
	}
	if (_argu[0] != OPER_NAME)
	{
		reply_send(ERR_NOOPERHOST, "", _sender);
		return (0);
	}
	if (_argu[1] != OPER_PASSWD)
	{
		reply_send(ERR_PASSWDMISMATCH, "", _sender);
		return (0);
	}
	_sender->set_user_state(OPERATOR);
	reply_send(RPL_YOUREOPER, "", _sender);
	return (0);
}

int Operator::_kick(void)
{
	if (_argu.size() < 2)
	{
		reply_send(ERR_NEEDMOREPARAMS, _command_str, _sender);
		return (0);
	}
	ChanRef kick_chan = _info.find_chan(_argu[0]);
	if (!kick_chan)
	{
		reply_send(ERR_NOSUCHCHANNEL, _argu[0], _sender);
		return (0);
	}
	std::stringstream kick_list(_argu[1]);
	std::string kick_name;
	std::getline(kick_list, kick_name, ',');
	while (kick_list)
	{
		ClientRef kick_cl = _info.find_client(kick_name);
		if (!kick_cl)
			reply_send(ERR_NOTONCHANNEL, _argu[0], _sender);
		else
		{
			int res = _info.kick_chan(kick_chan, kick_cl, _argu[2], _sender);
			if (res == ERR_USERNOTINCHANNEL)
				reply_send(res, _argu[0] + " " + _argu[1], _sender);
			else if (res)
				reply_send(res, _argu[0], _sender);
		}
		std::getline(kick_list, kick_name, ',');
	}
	return (0);
}

int Operator::_invite(void)
{
	if (_argu.size() < 2)
	{
		reply_send(ERR_NEEDMOREPARAMS, _command_str, _sender);
		return (0);
	}
	ClientRef invite_cl = _info.find_client(_argu[0]);
	if (!invite_cl)
	{
		reply_send(ERR_NOSUCHNICK, _argu[0], _sender);
		return (0);
	}
	ChanRef invite_chan = _info.find_chan(_argu[1]);
	int err = invite_chan->add_invite(invite_cl, _sender);
	if (err)
	{
		if (err == ERR_USERONCHANNEL)
			reply_send(ERR_USERONCHANNEL, _argu[0] + " " + _argu[1], _sender);
		else
			reply_send(err, _argu[1], _sender);
		return (0);
	}
	std::stringstream send_msg;
	send_msg << _sender->get_prifix() << " INVITE " << _argu[0] << " " << _argu[1];
	std::string invite_msg = send_msg.str();
	invite_cl->add_output(invite_msg);
	reply_send(RPL_INVITING, _argu[0] + " " + _argu[1], _sender);
	return (1);
}

int Operator::_mode(void)
{
	if (_argu.empty())
	{
		reply_send(ERR_NEEDMOREPARAMS, _command_str, _sender);
		return (0);
	}
	if (_argu[0][0] != '#')
		return (0);
	ChanRef mode_chan = _info.find_chan(_argu[0]);
	if (!mode_chan)
	{
		reply_send(ERR_NOSUCHCHANNEL, _argu[0], _sender);
		return (0);
	}
	if (_argu.size() == 1)
	{
		reply_send(RPL_CHANNELMODEIS, mode_chan->get_chanmode_str(), _sender);
		reply_send(RPL_CREATIONTIME, mode_chan->get_chantime_str(), _sender);
		return (1);
	}

	int len = _argu[1].length();
	int arg_len = _argu.size();
	int arg_idx = 2;
	bool flag = true;
	std::stringstream mode_str;
	for (int i = 0; i < len; i++)
	{
		switch (_argu[1][i])
		{
		case 'o':
		case 'k':
			if (arg_idx == arg_len)
				break;
			arg_idx++;
			mode_str << _argu[1][i];
			break;
		case '+':
			flag = true;
			mode_str << _argu[1][i];
			break;
		case '-':
			flag = false;
			mode_str << _argu[1][i];
			break;
		case 'i':
		case 't':
			mode_str << _argu[1][i];
			break;
		case 'l':
			if (flag)
			{
				if (arg_idx == arg_len)
					break ;
				else
					arg_idx++;
			}
			mode_str << _argu[1][i];
			break;
		default:
			reply_send(ERR_UNKNOWNMODE, (char[2]){_argu[1][i], 0}, _sender);
			break;
		}
	}
	if (mode_str.str().empty())
		return (0);
	int err = mode_chan->mode_set(mode_str.str(), _argu, _sender);
	if (err)
		reply_send(err, _argu[0], _sender);
	return (0);
}

int Operator::_topic(void)
{
	if (_argu.empty())
	{
		reply_send(ERR_NEEDMOREPARAMS, _command_str, _sender);
		return (0);
	}
	ChanRef topic_chan = _info.find_chan(_argu[0]);
	if (!topic_chan)
	{
		reply_send(ERR_NOSUCHCHANNEL, _argu[0], _sender);
		return (0);
	}
	if (_argu.size() == 1)
	{
		std::string topic = topic_chan->get_topic();
		if (!topic.empty())
			reply_send(RPL_TOPIC, topic, _sender);
		else
			reply_send(RPL_NOTOPIC, _argu[0], _sender);
	}
	else // 토픽 설정
	{
		int err = topic_chan->set_topic(_argu[1], _sender);
		if (err)
			reply_send(err, _argu[0], _sender);
	}
	return (0);
}

int Operator::_quit(void)
{
	std::stringstream quit_msg;
	std::stringstream error_msg;

	error_msg << "ERROR " << ":Closing Link [Quit: leaving]";
	if (!_argu.empty())
		quit_msg << _sender->get_prifix() << " QUIT :" << _argu[0];
	else
		quit_msg << _sender->get_prifix() << " QUIT :";
	_info.remove_client(_sender->get_fd(), quit_msg.str());
	_sender->add_output(error_msg.str());
	return (0);
}

int Operator::_part(void)
{
	if (_argu.empty())
	{
		reply_send(ERR_NEEDMOREPARAMS, _command_str, _sender);
		return (0);
	}
	std::stringstream chan(_argu[0]);
	std::string leave;
	std::getline(chan, leave, ',');
	while (chan)
	{
		ChanRef leave_chan = _info.find_chan(leave);
		if (!leave_chan)
			reply_send(ERR_NOSUCHCHANNEL, leave, _sender);
		else
		{
			std::stringstream part_msg;
			if (_argu.size() > 1)
				part_msg << _sender->get_prifix() << " PART " << leave << " " << ":" << _argu[1];
			else
				part_msg << _sender->get_prifix() << " PART " << leave;
			int err = _info.leave_chan(leave_chan, _sender, part_msg.str());
			if (err)
				reply_send(ERR_NOTONCHANNEL, leave, _sender);
		}
		std::getline(chan, leave, ',');
	}
	return (1);
}

int Operator::_ping(void)
{
	std::string pong_msg = ":";
	
	pong_msg += SERVER_NAME;
	pong_msg += " PONG ";
	pong_msg += SERVER_NAME;
	if (!_argu.empty())
	{
		pong_msg += " ";
		pong_msg += _argu[0];
	}
	_sender->add_output(pong_msg);
	return (1);
}

int Operator::reply_send(int reply, std::string param, ClientRef sender)
{
	std::stringstream cmd;

	cmd << ":" << SERVER_NAME << " ";
	switch (reply)
	{
	case ERR_ALREADYRGISTRED:
		cmd << "462 " << sender->get_nick() << " :You may not reregister";
		break;
	case ERR_NEEDMOREPARAMS:
		cmd << "461 " << sender->get_nick() << " " << param \
			<< " :Not enough parameters";
		break;
	case ERR_PASSWDMISMATCH:
		cmd << "464 " << sender->get_nick() << " :Password incorrect";
		break;
	case ERR_NICKNAMEINUSE:
		cmd << "433 " << sender->get_nick() << " " << param \
			<< " :Nickname is already in use.";
		break;
	case ERR_ERRONEUSNICKNAME:
		cmd << "432 " << sender->get_nick() << " " << param \
			<< " :Erroneus nickname";
		break;
	case ERR_NONICKNAMEGIVEN:
		cmd << "431 " << sender->get_nick() << " :No nickname given";
		break;
	case ERR_INVITEONLYCHAN:
		cmd << "473 " << sender->get_nick() << " " << param \
			<< " :Cannot join channel (+i)";
		break;
	case ERR_BADCHANNELKEY:
		cmd << "475 " << sender->get_nick() << " " << param \
			<< " :Cannot join channel (+k)";
		break;
	case ERR_CHANNELISFULL:
		cmd << "471 " << sender->get_nick() << " " << param \
			<< " :Cannot join channel (+l)";
		break;
	case ERR_NOSUCHCHANNEL:
		cmd << "403 " << sender->get_nick() << " " << param \
			<< " :No such channel";
		break;
	case ERR_NORECIPIENT:
		cmd << "411 " << sender->get_nick() 
			<< " :No recipient given (" << param << ")";
		break;
	case ERR_CANNOTSENDTOCHAN:
		cmd << "404 " << sender->get_nick() << " " << param \
			<< " :Cannot send to channel";
		break;
	case ERR_NOSUCHNICK:
		cmd << "401 " << sender->get_nick() << " " << param \
			<< " :No such nick/channel";
		break;
	case ERR_NOTEXTTOSEND:
		cmd << "412 " << sender->get_nick() << " :No text to send";
		break;
	case ERR_NOOPERHOST:
		cmd << "491 " << sender->get_nick() << " :No O-lines for your host";
		break;
	case ERR_USERNOTINCHANNEL:
		cmd << "441 " << sender->get_nick() << " " << param \
			<< " :They aren't on that channel";
		break;
	case ERR_NOTONCHANNEL:
		cmd << "442 " << sender->get_nick() << " " << param \
			<< " :You're not on that channel";
		break;
	case ERR_CHANOPRIVSNEEDED:
		cmd << "482 " << sender->get_nick() << " " << param \
			<< " :You're not channel operator";
		break;
	case ERR_USERONCHANNEL:
		cmd << "443 " << sender->get_nick() << " " << param \
			<< " :is already on channel";
		break;
	case ERR_UNKNOWNMODE:
		cmd << "472 " << sender->get_nick() << " " << param \
			<< " :is unknown mode char to me";
		break;
	case ERR_USERSDONTMATCH:
		cmd << "502 " << sender->get_nick() << " :Cant change mode for other users";
		break;
	case ERR_UMODEUNKNOWNFLAG:
		cmd << "501 " << sender->get_nick() << " :Unknown MODE flag";
		break;

	case RPL_WELCOME:
		cmd << "001 " << sender->get_nick() << " :Welcome to the 42_IRC Network, " << sender->get_prifix();
		break;
	case RPL_YOURHOST:
		cmd << "002 " << sender->get_nick() << " :Your host is " << SERVER_NAME << ", running version v0.1";
		break;
	case RPL_TOPIC:
		cmd << "332 " << sender->get_nick() << " " << param;
		break;
	case RPL_NAMREPLY:
		cmd << "353 " << sender->get_nick() << " " << param;
		break;
	case RPL_ENDOFNAMES:
		cmd << "366 " << sender->get_nick() << " " << param << " :End of /NAMES list";
		break;
	case RPL_YOUREOPER:
		cmd << "381 " << sender->get_nick() << " :You are now an IRC operator";
		break;
	case RPL_NOTOPIC:
		cmd << "331 " << sender->get_nick() << " " << param << " :No topic is set";
		break;
	case RPL_INVITING:
		cmd << "341 " << sender->get_nick() << " " << param; // doc 참조
		break;
	case RPL_CHANNELMODEIS:
		cmd << "324 " << sender->get_nick() << " " << param; // doc 참조
		break;
	case RPL_CREATIONTIME:
		cmd << "329 " << sender->get_nick() << " " << param; // doc 참조
		break;
	case RPL_UMODEIS:
		cmd << "221 " << sender->get_nick() << " " << param;
		break;
	default:
		return (0);
	}
	std::string reply_str = cmd.str();
	std::cout << "[Debug]: reply: " << reply_str << '\n' << std::endl;
	sender->add_output(reply_str);
	return (1);
}