#pragma once

#ifndef EVENT_HPP
# define EVENT_HPP

#include "Client.hpp"
#include "SharedPtr.hpp"

class Event
{
private:
	virtual int Handler(void) = 0;
	ClientRef getClient(void);
private:
	ClientRef cl;
}; 

class ReadEvent : public Event
{
	int Handler();
};

class WriteEvent : public Event
{
	int Handler();
};

class ListenEvent : public Event
{
	int Handler();
};

class ErrorEvent : public Event
{
	int Handler();
};

#endif
