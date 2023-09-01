#pragma once

#ifndef EVENTQ_HPP
# define EVENTQ_HPP

#include <sys/time.h>
#include <sys/event.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <pthread.h>

#define NOTIFY_IDENT 42


typedef struct kevent t_event;

class Eventq
{
public:
	static Eventq &getInstance(void);

	int init(void);
	int reg_event(int socket, int16_t filter, uint16_t flag, uint16_t fflage, intptr_t data, void *udata);
	int get_event(t_event event[], int len);
	void destroy_eventq(void);
private:
	int _kq;
	std::vector<t_event> _change_list;
	pthread_mutex_t _mutex_change_list;

	void _forced_trigger(void);
	
	Eventq	&operator=(const Eventq &copy);
	Eventq(const Eventq &copy);
	Eventq(void);
	~Eventq(void);
};

#endif
