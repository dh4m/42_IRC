#include "Channel.hpp"
#include "Operator.hpp"

Channel::Channel(std::string name)
: _name(name), _create_time(time(NULL)), _mode(0), _user_limit(-1)
{
	pthread_mutex_init(&_channel_m, NULL);
}

Channel::~Channel(void)
{
	pthread_mutex_destroy(&_channel_m);
}

const std::string &Channel::get_name(void) const
{
	return (_name);
}

int Channel::add_user(ClientRef client, const std::string &key)
{
	if (!client)
		return (0);
	ScopeLock<pthread_mutex_t> lock(&_channel_m);
	if (!_is_invite(client)
		&& _user_limit <= _member.size())
		return (ERR_CHANNELISFULL);
	else if (_mode.test(INVITEONLY) 
			&& !_is_invite(client))
		return (ERR_INVITEONLYCHAN);
	else if (!_is_invite(client)
			&& !_passwd.empty() && _passwd != key)
		return (ERR_BADCHANNELKEY);
	if (_is_invite(client))
		_invite_list.erase(client);
	_member.insert(client);
	return (0);
}

int Channel::add_operator(ClientRef client)
{
	if (!client)
		return (0);
	ScopeLock<pthread_mutex_t> lock(&_channel_m);
	_operator.insert(client);
	return (0);
}

int Channel::remove_user(ClientRef client)
{
	ScopeLock<pthread_mutex_t> lock(&_channel_m);
	_member.erase(client);
	_operator.erase(client);
	_invite_list.erase(client);
	return (0);
}

int Channel::remove_operator(ClientRef client)
{
	ScopeLock<pthread_mutex_t> lock(&_channel_m);
	_operator.erase(client);
	return (0);
}

int Channel::is_user(ClientRef client)
{
	ScopeLock<pthread_mutex_t> lock(&_channel_m);
	return (_is_user(client));
}

int Channel::is_operator(ClientRef client)
{
	ScopeLock<pthread_mutex_t> lock(&_channel_m);
	return (_is_operator(client));
}

int Channel::is_inviteonly(void)
{
	ScopeLock<pthread_mutex_t> lock(&_channel_m);
	return (_mode.test(INVITEONLY));
}

int Channel::is_needpasswd(void)
{
	ScopeLock<pthread_mutex_t> lock(&_channel_m);
	return (!_passwd.empty());
}

int Channel::passwd_check(std::string passwd)
{
	ScopeLock<pthread_mutex_t> lock(&_channel_m);
	return (_passwd == passwd);
}

int Channel::add_invite(ClientRef user, ClientRef op)
{
	ScopeLock<pthread_mutex_t> lock(&_channel_m);
	if (!_is_user(op))
		return (ERR_NOTONCHANNEL);
	if (!_is_operator(op))
		return (ERR_CHANOPRIVSNEEDED);
	if (_is_user(user))
		return (ERR_USERONCHANNEL);
	_invite_list.insert(user);
	return (0);
}

int Channel::del_invite(ClientRef user)
{
	ScopeLock<pthread_mutex_t> lock(&_channel_m);
	_invite_list.erase(user);
	return (0);
}

int Channel::is_invite_memb(ClientRef user)
{
	ScopeLock<pthread_mutex_t> lock(&_channel_m);
	return (_is_invite(user));
}

int Channel::limit_memb_check(void)
{
	ScopeLock<pthread_mutex_t> lock(&_channel_m);
	return (_user_limit > _member.size());
}

size_t Channel::limit_memb(void)
{
	ScopeLock<pthread_mutex_t> lock(&_channel_m);
	return (_user_limit);
}

size_t Channel::curr_memb(void)
{
	ScopeLock<pthread_mutex_t> lock(&_channel_m);
	return (_member.size());
}

int Channel::kick_user(ClientRef kick, const std::string &msg, ClientRef op)
{
	ScopeLock<pthread_mutex_t> lock(&_channel_m);
	if (!_is_user(op))
		return (ERR_NOTONCHANNEL);
	if (!_is_operator(op))
		return (ERR_CHANOPRIVSNEEDED);
	if (!_is_user(kick))
		return (ERR_USERNOTINCHANNEL);
	std::stringstream kick_msg;
	kick_msg << op->get_prifix() << " KICK " << _name << " "
			<< kick->get_nick() << " :" << msg;
	std::string kick_str = kick_msg.str();
	_channel_output(kick_str, ClientRef::NullSharedPtr());
	_member.erase(kick);
	_operator.erase(kick);
	_invite_list.erase(kick);
	return (0);
}

int Channel::set_topic(std::string topic, ClientRef op)
{
	ScopeLock<pthread_mutex_t> lock(&_channel_m);
	if (!_is_user(op))
		return (ERR_NOTONCHANNEL);
	if (_mode.test(TOPICRESTRICT) && !_is_operator(op))
		return (ERR_CHANOPRIVSNEEDED);
	_topic = topic;
	std::stringstream topic_msg;
	topic_msg << op->get_prifix() << " TOPIC " << _name << " :" << topic;
	_channel_output(topic_msg.str(), ClientRef::NullSharedPtr());
	return (0);
}

int Channel::set_limit(size_t limit, ClientRef op)
{
	ScopeLock<pthread_mutex_t> lock(&_channel_m);
	if (!_is_user(op))
		return (ERR_NOTONCHANNEL);
	if (!_is_operator(op))
		return (ERR_CHANOPRIVSNEEDED);
	_user_limit = limit;
	return (0);
}

int Channel::set_passwd(std::string passwd, ClientRef op)
{
	ScopeLock<pthread_mutex_t> lock(&_channel_m);
	if (!_is_user(op))
		return (ERR_NOTONCHANNEL);
	if (!_is_operator(op))
		return (ERR_CHANOPRIVSNEEDED);
	_passwd = passwd;
	return (0);
}

int Channel::mode_set(std::string modestr, std::vector<std::string> &args, ClientRef op)
{
	ScopeLock<pthread_mutex_t> lock(&_channel_m);
	if (!_is_user(op))
		return (ERR_NOTONCHANNEL);
	if (!_is_operator(op))
		return (ERR_CHANOPRIVSNEEDED);
	std::stringstream change_str;
	if (args.size() == 2)
		change_str << ":";
	std::stringstream args_str;
	bool flag = true;
	ClientRef oper_cl;
	size_t oper_limit = -1;
	size_t arg_idx = 2;
	bool now_set = false;
	for (size_t i = 0; i < modestr.length(); i++)
	{
		switch (modestr[i])
		{
		case '+':
			if (!flag || i == 0)
				change_str << "+";
			flag = true;
			break;
		case '-':
			if (flag || i == 0)
				change_str << "-";
			flag = false;
			break;
		case 'o':
			oper_cl = _find_member_nick(args[arg_idx]);
			if (!oper_cl)
			{
				Operator::reply_send(ERR_NOSUCHNICK, args[arg_idx++], op);
				break ;
			}
			if (flag && !_is_operator(oper_cl))
			{
				_operator.insert(oper_cl);
				change_str << "o";
				if (arg_idx < args.size() - 1)
					args_str << args[arg_idx++] << " ";
				else if (arg_idx == args.size() - 1)
					args_str << ":" << args[arg_idx++];
				now_set = true;
			}
			else if (!flag && _is_operator(oper_cl))
			{
				_operator.erase(oper_cl);
				change_str << "o";
				if (arg_idx < args.size() - 1)
					args_str << args[arg_idx++] << " ";
				else if (arg_idx == args.size() - 1)
					args_str << ":" << args[arg_idx++];
				now_set = true;
			}
			break;
		case 'i':
			if (_mode.test(INVITEONLY) == flag)
				break ;
			change_str << "i";
			now_set = true;
			_mode.set(INVITEONLY, flag);
			break;
		case 't':
			if (_mode.test(TOPICRESTRICT) == flag)
				break ;
			change_str << "t";
			now_set = true;
			_mode.set(TOPICRESTRICT, flag);
			break;
		case 'l':
			if (flag)
			{
				oper_limit = strtol(args[arg_idx].c_str(), NULL, 10);
				if (oper_limit < 0 || oper_limit == _user_limit)
				{
					arg_idx++;
					break;
				}
				_user_limit = oper_limit;
				change_str << "l";
				if (arg_idx < args.size() - 1)
					args_str << args[arg_idx++] << " ";
				else if (arg_idx == args.size() - 1)
					args_str << ":" << args[arg_idx++];
				now_set = true;
			}
			else if (!flag && (_user_limit != static_cast<unsigned long>(-1)))
			{
				_user_limit = -1;
				change_str << "l";
				if (arg_idx < args.size() - 1)
					args_str << args[arg_idx++] << " ";
				else if (arg_idx == args.size() - 1)
					args_str << ":" << args[arg_idx++];
				now_set = true;
			}
			break;
		case 'k':
			if (flag && _passwd.empty())
			{
				_passwd = args[arg_idx];
				change_str << "k";
				if (arg_idx < args.size() - 1)
					args_str << args[arg_idx++] << " ";
				else if (arg_idx == args.size() - 1)
					args_str << ":" << args[arg_idx++];
				now_set = true;
			}
			else if (!flag && args[arg_idx] == _passwd)
			{
				_passwd.clear();
				change_str << "k";
				if (arg_idx < args.size() - 1)
					args_str << args[arg_idx++] << " ";
				else if (arg_idx == args.size() - 1)
					args_str << ":" << args[arg_idx++];
				now_set = true;
			}
			break;
		}
	}
	if (now_set)
	{
		std::stringstream mode_msg;
		mode_msg << op->get_prifix() << " MODE " << _name << " "
				<< change_str.str() << " " << args_str.str();
		_channel_output(mode_msg.str(), ClientRef::NullSharedPtr());
	}
	return (0);
}

int Channel::channel_output(const std::string &content, ClientRef talker)
{
	ScopeLock<pthread_mutex_t> lock(&_channel_m);
	if (!_is_user(talker))
		return (ERR_CANNOTSENDTOCHAN);
	_channel_output(content, talker);
	return (0);
}

int Channel::channel_output_someone(const std::string &content, ClientRef talker, std::set<ClientRef> &send)
{
	ScopeLock<pthread_mutex_t> lock(&_channel_m);
	if (!_is_user(talker))
		return (0);
	for (std::set<ClientRef>::iterator it = _member.begin();
		it != _member.end();
		++it)
	{
		if (send.find(*it) == send.end() && *it != talker)
		{
			(*it)->add_output(content);
			send.insert(*it);
		}
	}
	return (0);
}

std::string Channel::get_topic(void)
{
	ScopeLock<pthread_mutex_t> lock(&_channel_m);
	if (_topic.empty())
		return ("");
	std::stringstream ss;

	ss << _name << " :" << _topic;
	return (ss.str());
}

std::string Channel::get_namelist_reply(void)
{
	ScopeLock<pthread_mutex_t> lock(&_channel_m);
	std::stringstream ss;

	ss << "= " << _name << " :";
	for (std::set<ClientRef>::iterator it = _member.begin();
		it != _member.end();
		++it)
	{
		if (it != _member.begin())
			ss << " ";
		if (_is_operator(*it))
			ss << "@" << (*it)->get_nick();
		else
			ss << (*it)->get_nick();
	}
	return (ss.str());
}

std::string Channel::get_chanmode_str(void)
{
	ScopeLock<pthread_mutex_t> lock(&_channel_m);
	std::stringstream chanmode_msg;

	chanmode_msg << _name << " " << ":+n";
	if (_mode.test(INVITEONLY))
		chanmode_msg << "i";
	if (_mode.test(TOPICRESTRICT))
		chanmode_msg << "t";
	if (!_passwd.empty())
		chanmode_msg << "k";
	if (_user_limit != static_cast<unsigned long>(-1))
		chanmode_msg << "l";
	return (chanmode_msg.str());
}

std::string Channel::get_chantime_str(void)
{
	std::stringstream chantime_msg;

	chantime_msg << _name << " " << ":" << _create_time;
	return (chantime_msg.str());
}

int Channel::_is_user(ClientRef client)
{
	return (client->get_user_state() == OPERATOR || _member.find(client) != _member.end());
}

int Channel::_is_operator(ClientRef client)
{
	return (client->get_user_state() == OPERATOR || _operator.find(client) != _operator.end());
}

int Channel::_is_invite(ClientRef client)
{
	return (_invite_list.find(client) != _invite_list.end());
}

ClientRef Channel::_find_member_nick(std::string &nick)
{
	for (std::set<ClientRef>::iterator it = _member.begin();
		it != _member.end();
		it++)
	{
		if ((*it)->get_nick() == nick)
			return (*it);
	}
	return (ClientRef::NullSharedPtr());
}

int Channel::_channel_output(const std::string &content, ClientRef talker)
{
	for (std::set<ClientRef>::iterator it = _member.begin();
		it != _member.end();
		++it)
	{
		if (*it != talker)
			(*it)->add_output(content);
	}
	return (0);
}
