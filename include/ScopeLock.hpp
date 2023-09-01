#pragma once

#ifndef SCOPELOCK_HPP
# define SCOPELOCK_HPP

#include <pthread.h>

template <typename T = pthread_mutex_t>
class ScopeLock
{
public:
	ScopeLock(pthread_mutex_t *lock)
	: _lock(lock)
	{
		if (_lock)
			pthread_mutex_lock(_lock);
	}

	~ScopeLock(void)
	{
		if (_lock)
			pthread_mutex_unlock(_lock);
	}

private:
	pthread_mutex_t *_lock;

	ScopeLock	&operator=(const ScopeLock &copy);
	ScopeLock(const ScopeLock &copy);
};

class WriteLock {};
class ReadLock {};

template <>
class ScopeLock<pthread_rwlock_t>
{
public:
	ScopeLock(pthread_rwlock_t *lock, WriteLock type)
	: _lock(lock)
	{
		(void) type;
		if (_lock)
			pthread_rwlock_wrlock(_lock);
	}

	ScopeLock(pthread_rwlock_t *lock, ReadLock type)
	: _lock(lock)
	{
		(void) type;
		if (_lock)
			pthread_rwlock_rdlock(_lock);
	}

	~ScopeLock(void)
	{
		if (_lock)
			pthread_rwlock_unlock(_lock);
	}

private:
	pthread_rwlock_t *_lock;

	ScopeLock	&operator=(const ScopeLock &copy);
	ScopeLock(const ScopeLock &copy);
};

#endif
