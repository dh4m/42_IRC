#include "Eventq.hpp"
#include <iostream>
#include <fcntl.h>
#include "ScopeLock.hpp"

Eventq::Eventq(void): _kq(-1) {}
Eventq::~Eventq(void) {}

Eventq &Eventq::getInstance(void)
{
	static Eventq singleton_obj;
	return (singleton_obj);
}

int Eventq::init(void)
{
	t_event temp;

	_kq = kqueue();
	if (_kq == -1)
		return (0);
	pthread_mutex_init(&_mutex_change_list, NULL);
	EV_SET(&temp, NOTIFY_IDENT, EVFILT_USER, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, NULL);
	kevent(_kq, &temp, 1, NULL, 0, NULL);
	EV_SET(&temp, SIGINT, EVFILT_SIGNAL, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, NULL);
	kevent(_kq, &temp, 1, NULL, 0, NULL);
	return (1);
}

int Eventq::reg_event(int socket, int16_t filter, uint16_t flag, uint16_t fflage, intptr_t data, void *udata)
{
	t_event temp;

	EV_SET(&temp, socket, filter, flag, fflage, data, udata);
	{
		ScopeLock<pthread_mutex_t> lock(&_mutex_change_list);
		_change_list.push_back(temp);
	}
	_forced_trigger();
	return (1);
}

int Eventq::get_event(t_event event[], int len)
{
	int event_num;
	std::vector<t_event> tmp_list;
	
	do
	{
		{
			ScopeLock<pthread_mutex_t> lock(&_mutex_change_list);
			if (_change_list.size())
			{
				tmp_list = _change_list;
				_change_list.clear();
			}
		}
		event_num = kevent(_kq, tmp_list.data(), tmp_list.size(), event, len, NULL);
	} while (event_num == 1 && event[0].filter == EVFILT_USER && event[0].ident == NOTIFY_IDENT);
	return (event_num);
}

void Eventq::destroy_eventq(void)
{
	if (_kq == -1)
		return ;
	close(_kq);
	pthread_mutex_destroy(&_mutex_change_list);
}

void Eventq::_forced_trigger(void)
{
	t_event trigger;

	EV_SET(&trigger, NOTIFY_IDENT, EVFILT_USER, 0, NOTE_TRIGGER, 0, NULL);
	kevent(_kq, &trigger, 1, NULL, 0, NULL);
}
