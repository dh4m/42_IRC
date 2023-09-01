#pragma once

#ifndef SHAREDPTR_HPP
# define SHAREDPTR_HPP

#include <pthread.h>

template <typename T>
struct DataChunk
{
	template <typename U>
	DataChunk(U arg): _data(arg), _cnt(1) {}

	T	_data;
	pthread_mutex_t _mut_cnt;
	int		_cnt;
};

template <typename T>
class SharedPtr
{
public:
	SharedPtr(void): _alloc_data(NULL) {}

	SharedPtr(const SharedPtr &copy)
	: _alloc_data(copy._alloc_data)
	{
		if (!_alloc_data)
			return ;
		pthread_mutex_lock(&_alloc_data->_mut_cnt);
		_alloc_data->_cnt++;
		pthread_mutex_unlock(&_alloc_data->_mut_cnt);
	}

	~SharedPtr(void)
	{
		if (_alloc_data)
		{
			pthread_mutex_lock(&_alloc_data->_mut_cnt);
			_alloc_data->_cnt--;
			if (_alloc_data->_cnt == 0)
			{
				pthread_mutex_unlock(&_alloc_data->_mut_cnt);
				_delete_object();
			}
			else
				pthread_mutex_unlock(&_alloc_data->_mut_cnt);
		}
	}

	SharedPtr<T>	&operator=(const SharedPtr &copy)
	{
		if (_alloc_data == copy._alloc_data)
			return (*this);
		if (_alloc_data)
		{
			pthread_mutex_lock(&_alloc_data->_mut_cnt);
			_alloc_data->_cnt--;
			if (_alloc_data->_cnt == 0)
			{
				pthread_mutex_unlock(&_alloc_data->_mut_cnt);
				_delete_object();
			}
			else
				pthread_mutex_unlock(&_alloc_data->_mut_cnt);
		}
		_alloc_data = copy._alloc_data;
		if (_alloc_data)
		{
			pthread_mutex_lock(&_alloc_data->_mut_cnt);
			_alloc_data->_cnt++;
			pthread_mutex_unlock(&_alloc_data->_mut_cnt);
		}
		return (*this);
	}

	T	&operator*(void) const { return (_alloc_data->_data); }

	T	*operator->(void) const { return (&_alloc_data->_data); }

	bool	operator==(const SharedPtr &cmp) const
	{
		return (_alloc_data == cmp._alloc_data);
	}

	bool	operator<=(const SharedPtr &cmp) const
	{
		return (_alloc_data <= cmp._alloc_data);
	}

	bool	operator<(const SharedPtr &cmp) const
	{
		return (_alloc_data < cmp._alloc_data);
	}

	bool	operator>=(const SharedPtr &cmp) const
	{
		return (_alloc_data >= cmp._alloc_data);
	}

	bool	operator>(const SharedPtr &cmp) const
	{
		return (_alloc_data > cmp._alloc_data);
	}

	bool	operator!=(const SharedPtr &cmp) const
	{
		return (_alloc_data != cmp._alloc_data);
	}

	bool	operator!(void) const
	{
		return (_alloc_data == NULL);
	}

	operator bool() const
	{
		return (_alloc_data != NULL);
	}

	static const SharedPtr<T>& NullSharedPtr(void)
	{
		static SharedPtr<T> null_pointer;
		return (null_pointer);
	}

	template <typename U, typename Args>
	friend SharedPtr<U> make_SharedPtr(Args arg);

private:
	void _delete_object(void)
	{
		pthread_mutex_destroy(&_alloc_data->_mut_cnt);
		delete _alloc_data;
		_alloc_data = NULL;
	}

	DataChunk<T> *_alloc_data;
};

template <typename T, typename Args>
SharedPtr<T> make_SharedPtr(Args arg)
{
	SharedPtr<T> ret_ptr;

	ret_ptr._alloc_data = new DataChunk<T>(arg);
	pthread_mutex_init(&ret_ptr._alloc_data->_mut_cnt, NULL);
	return (ret_ptr);
}

#endif
