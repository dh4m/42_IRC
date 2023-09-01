#include <iostream>

#include "Bot.hpp"

void invalid_input(void)
{
	std::cerr << "Invalid input!" << std::endl;
	std::cerr << "Usage: ./ircbot servip port password" << std::endl;
	exit(EXIT_FAILURE);
}

bool isnum(char *str)
{
	int i = 0;

	while (str[i])
	{
		if (!isdigit(str[i]))
			return (false);
		i++;
	}
	return (true);
}

void server_close_signal(int sig)
{
	(void)sig;
	exit(0);
}

int main(int argc, char *argv[])
{
	if (argc != 4 || !isnum(argv[2]) || inet_addr(argv[1]) == INADDR_NONE)
		invalid_input();
	signal(SIGINT, server_close_signal);

	Bot bot;

	if (!bot.connent_serv(inet_addr(argv[1]), atoi(argv[2]), argv[3]))
	{
		std::cerr << "Bot-Server Connect Error!" << std::endl;
		exit(1);
	}
	if (!bot.run())
	{
		std::cerr << "Bot runtime Error!" << std::endl;
		exit(1);
	}
	return (0);
}