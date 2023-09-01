#include "Worker.hpp"
#include "Eventq.hpp"
#include "Operator.hpp"
#include "ScopeLock.hpp"
#include <iostream>

Worker::Worker(void)
{
	pthread_cond_init(&_msgQ._q_fill_cond, NULL);
	pthread_mutex_init(&_msgQ._msgq_m, NULL);
	_thread_list.resize(NUM_THREAD);
}

int Worker::_worker_status = 1;

Worker::~Worker(void)
{
	pthread_cond_destroy(&_msgQ._q_fill_cond);
	pthread_mutex_destroy(&_msgQ._msgq_m);
}

int Worker::init(const std::string &passwd)
{
	_passwd = passwd;
	for(int i = 0; i < NUM_THREAD; i++)
	{
		pthread_create(&_thread_list[i], NULL, \
						_worker_thread_func, this);
	}
	return (1);
}

void Worker::add_client(int fd)
{
	_cl_info.add_client(fd);
}

void Worker::remove_client(int fd, const char *msg)
{
	_cl_info.remove_client(fd, msg);
}

void Worker::close_client(int fd)
{
	_cl_info.close_client(fd);
}

void Worker::reg_msg(int fd, int filt, long id)
{
	t_msg insert_msg = {.fd = fd, .filt = filt, .id = id};
	Eventq & ev_q = Eventq::getInstance();

	ev_q.reg_event(fd, filt, EV_DISABLE, 0, 0, reinterpret_cast<void *>(id));
	ScopeLock<pthread_mutex_t> lock(&_msgQ._msgq_m);
	_msgQ._messageQ.push_back(insert_msg);
	pthread_cond_signal(&_msgQ._q_fill_cond);
}

int Worker::destroy(void)
{
	_worker_status = 0;
	pthread_cond_broadcast(&_msgQ._q_fill_cond);
	for (int i = 0; i < NUM_THREAD; i++)
		pthread_join(_thread_list[i], NULL);
	return (0);
}

void *Worker::_worker_thread_func(void *args)
{
	Worker &worker = *reinterpret_cast<Worker *>(args);
	t_messageQ &q = worker._msgQ;
	t_msg curr_msg;
	ClientRef op_cl;
	Eventq &ev_q = Eventq::getInstance();
	Operator operate_cmd(worker._cl_info, worker._passwd);
	std::string input;

	while (_worker_status)
	{
		op_cl = ClientRef::NullSharedPtr();
		pthread_mutex_lock(&q._msgq_m);
		while (q._messageQ.empty())
		{
			pthread_cond_wait(&q._q_fill_cond, &q._msgq_m);
			if (!_worker_status)
			{
				pthread_mutex_unlock(&q._msgq_m);
				return (NULL);
			}
		}
		curr_msg = q._messageQ.front();
		q._messageQ.pop_front();
		pthread_mutex_unlock(&q._msgq_m);
		op_cl = worker._cl_info.find_client(curr_msg.fd);
		if (!op_cl || op_cl->get_id() != curr_msg.id) // 클라이언트 id 검사 부분
			continue;
		if (op_cl->get_user_state() == UNAVAIL_USER && curr_msg.filt != EVFILT_WRITE)
			continue;
		if (curr_msg.filt == EVFILT_READ)
		{
			int res = op_cl->client_read();
			if (res == DISCONNECT)
			{
				std::stringstream quit_msg;
				quit_msg << op_cl->get_prifix() << " QUIT :Connection closed";
				worker._cl_info.remove_client(op_cl->get_fd(), quit_msg.str());
				worker._cl_info.close_client(op_cl->get_fd());
				continue;
			}
			op_cl->get_input_buffer(input);
			while (!input.empty())
			{
				operate_cmd.cmd_proc(input, op_cl);
				op_cl->get_input_buffer(input);
			}
			if (op_cl->get_user_state() != UNAVAIL_USER)
				ev_q.reg_event(op_cl->get_fd(), EVFILT_READ, EV_ENABLE, 0, 0, reinterpret_cast<void *>(op_cl->get_id())); 
		}
		else if (curr_msg.filt == EVFILT_WRITE)
		{
			op_cl->client_write();
			if (op_cl->get_user_state() == UNAVAIL_USER && !op_cl->exist_output())
				worker._cl_info.close_client(op_cl->get_fd());
			if (op_cl->exist_output() && op_cl->get_user_state() != CLOSE_USER)
				ev_q.reg_event(op_cl->get_fd(), EVFILT_WRITE, EV_ENABLE, 0, 0, reinterpret_cast<void *>(op_cl->get_id()));
		}
	}
	op_cl = ClientRef::NullSharedPtr();
	return (NULL);
}