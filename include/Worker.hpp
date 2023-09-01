#pragma once

#ifndef WORKER_HPP
# define WORKER_HPP

#include <deque>
#include <map>
#include <set>
#include <string>

#include "Channel.hpp"
#include "ClientInfo.hpp"

#define NUM_THREAD 12

typedef struct s_msg
{
	int fd;
	int filt;
	int id;
} t_msg;

typedef struct s_messageQ
{
	std::deque<t_msg> _messageQ;
	pthread_mutex_t _msgq_m;
	pthread_cond_t _q_fill_cond;
}	t_messageQ;

class Worker
{
public:
	Worker(void);
	~Worker(void);

	int init(const std::string &passwd);
	void add_client(int fd);
	void remove_client(int fd, const char *msg);
	void close_client(int fd);
	void reg_msg(int fd, int filt, long id);
	int destroy(void);
private:
	static void *_worker_thread_func(void *args);
	static int _worker_status;

	std::string _passwd;
	ClientInfo _cl_info;
	t_messageQ _msgQ;
	std::vector<pthread_t> _thread_list;

	Worker(const Worker &copy);
	Worker	&operator=(const Worker &copy);
};

#endif
