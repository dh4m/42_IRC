#include "ClientInfo.hpp"
#include "Eventq.hpp"
#include "Operator.hpp"
#include "ScopeLock.hpp"

#include <unistd.h>
#include <fcntl.h>
#include <iostream>

long ClientInfo::_client_id_cnt = 0;

ClientInfo::ClientInfo(void)
{
	pthread_rwlock_init(&_cl_list_lock, NULL);
	pthread_rwlock_init(&_ch_list_lock, NULL);
}

ClientInfo::~ClientInfo(void)
{
	pthread_rwlock_destroy(&_cl_list_lock);
	pthread_rwlock_destroy(&_ch_list_lock);
}

int ClientInfo::add_client(int fd)
{
	Eventq &ev_q = Eventq::getInstance();
	ClientRef cl = make_SharedPtr<Client>(fd);

	std::cout << "[Debug]: " << fd << " is new client" << std::endl;
	fcntl(fd, F_SETFL, O_NONBLOCK);
	_client_id_cnt++;
	cl->id_set(_client_id_cnt);
	{
		ScopeLock<pthread_rwlock_t> lock(&_cl_list_lock, WriteLock());
		_cl_list.insert(std::make_pair(fd, cl));
	}
	ev_q.reg_event(fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, reinterpret_cast<void *>(cl->get_id()));
	ev_q.reg_event(fd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, reinterpret_cast<void *>(cl->get_id()));
	return (0);
}

int ClientInfo::reg_client(ClientRef cl, std::string nick)
{
	cl->set_user_state(AVAIL_USER);
	{
		ScopeLock<pthread_rwlock_t> lock(&_cl_list_lock, WriteLock());
		_cl_nick_list.insert(std::make_pair(nick, cl));
	}
	return (0);
}

ClientRef ClientInfo::find_client(int fd)
{
	ScopeLock<pthread_rwlock_t> lock(&_cl_list_lock, ReadLock());
	t_clientlist::const_iterator it = _cl_list.find(fd);
	if (it == _cl_list.end() || it->second->get_user_state() == CLOSE_USER)
		return (ClientRef::NullSharedPtr());
	return (it->second);
}

ClientRef ClientInfo::find_client(std::string nick)
{
	ScopeLock<pthread_rwlock_t> lock(&_cl_list_lock, ReadLock());
	t_nicklist::const_iterator it = _cl_nick_list.find(nick);
	if (it == _cl_nick_list.end() || it->second->get_user_state() == UNAVAIL_USER)
		return (ClientRef::NullSharedPtr());
	return (it->second);
}

void ClientInfo::remove_client(int fd, const std::string &msg)
{
	Eventq &ev_q = Eventq::getInstance();

	ev_q.reg_event(fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
	ClientRef remove_cl;
	{
		ScopeLock<pthread_rwlock_t> lock(&_cl_list_lock, WriteLock());
		std::map<int, ClientRef>::iterator it = _cl_list.find(fd);
		if (it == _cl_list.end())
			return ;
		_cl_nick_list.erase(it->second->get_nick());
		it->second->set_user_state(UNAVAIL_USER);
		remove_cl = it->second;
	}
	leave_all_channel(remove_cl, msg);
}

void ClientInfo::close_client(int fd)
{
	ScopeLock<pthread_rwlock_t> lock(&_cl_list_lock, WriteLock());
	std::map<int, ClientRef>::iterator it = _cl_list.find(fd);
	if (it == _cl_list.end())
		return ;
	it->second->set_user_state(CLOSE_USER);
	it->second->clear_buffer();
	_cl_list.erase(fd);
}

int ClientInfo::client_nick_change(int fd, std::string nick)
{
	std::stringstream nickmsg;
	t_clientlist::iterator it;
	{
		ScopeLock<pthread_rwlock_t> lock(&_cl_list_lock, WriteLock());
		it = _cl_list.find(fd);
		if (it == _cl_list.end())
			return (0);
		nickmsg << it->second->get_prifix() << " NICK " << nick;
		_cl_nick_list.erase(it->second->get_nick());
		it->second->nick_set(nick);
		_cl_nick_list.insert(std::make_pair(nick, it->second));
	}
	it->second->add_output(nickmsg.str());
	send_all_chan(nickmsg.str(), it->second);
	return (0);
}

int ClientInfo::nick_dup_check(std::string nick)
{
	if (!find_client(nick))
		return (0);
	return (1);
}

int ClientInfo::join_chan(const std::string &name, const std::string key, ClientRef ref)
{
	if (!ref)
		return (0);
	ScopeLock<pthread_rwlock_t> lock(&_ch_list_lock, WriteLock());
	ChanRef chan;
	t_chanlist::const_iterator it = _channel.find(name);
	if (it == _channel.end())
	{
		chan = make_SharedPtr<Channel>(name);
		chan->add_user(ref, "");
		chan->add_operator(ref);
		ref->add_chan(name);
		_channel.insert(make_pair(name, chan));
	}
	else
	{
		chan = it->second;
		int err = chan->add_user(ref, key);
		if (err)
			return (err);
		ref->add_chan(name);
	}
	return (0);
}

int ClientInfo::leave_chan(ChanRef chan, ClientRef ref, const std::string &msg)
{
	if (!chan || !ref)
		return (0);
	if (!chan->is_user(ref))
		return (ERR_NOTONCHANNEL);
	ScopeLock<pthread_rwlock_t> lock(&_ch_list_lock, WriteLock());
	ref->del_chan(chan->get_name());
	if (!msg.empty())
	{
		ref->add_output(msg);
		chan->channel_output(msg, ref);
	}
	chan->remove_user(ref);
	if (chan->curr_memb() == 0)
		_channel.erase(chan->get_name());
	return (0);
}

int ClientInfo::kick_chan(ChanRef chan, ClientRef kick, std::string &msg, ClientRef op)
{
	if (!chan || !kick || !op)
		return (0);
	ScopeLock<pthread_rwlock_t> lock(&_ch_list_lock, WriteLock());
	int res = chan->kick_user(kick, msg, op);
	if (res)
		return (res);
	kick->del_chan(chan->get_name());
	if (chan->curr_memb() == 0)
		_channel.erase(chan->get_name());
	return (0);
}


ChanRef ClientInfo::find_chan(const std::string &name)
{
	ScopeLock<pthread_rwlock_t> lock(&_ch_list_lock, ReadLock());
	t_chanlist::const_iterator it = _channel.find(name);
	if (it == _channel.end())
		return (ChanRef::NullSharedPtr());
	return (it->second);
}

int ClientInfo::leave_all_channel(ClientRef cl, const std::string &msg)
{
	if (!cl)
		return (0);
	ScopeLock<pthread_rwlock_t> lock(&_ch_list_lock, WriteLock());
	cl->clear_chan();
	for (t_chanlist::iterator it = _channel.begin();
		it != _channel.end();)
	{
		if (!msg.empty())
			(it->second)->channel_output(msg, cl);
		(it->second)->remove_user(cl);
		if ((it->second)->curr_memb() == 0)
			it = _channel.erase(it);
		else
			it++;
	}
	return (0);
}

int ClientInfo::send_all_chan(const std::string &msg, ClientRef op)
{
	ScopeLock<pthread_rwlock_t> lock(&_ch_list_lock, ReadLock());
	std::set<ClientRef> already_send;
	for (t_chanlist::iterator it = _channel.begin();
		it != _channel.end();
		it++)
		it->second->channel_output_someone(msg, op, already_send);
	return (0);
}
