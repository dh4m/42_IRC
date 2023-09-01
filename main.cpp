#include <iostream>
#include "Server.hpp"

void invalid_input(void)
{
	std::cerr << "Invalid input!" << std::endl;
	std::cerr << "Usage: ./ircserv port password" << std::endl;
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

int main(int argc, char *argv[])
{
	if (argc != 3 || !isnum(argv[1]))
		invalid_input();
	signal(SIGINT, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	Server serv(atoi(argv[1]), argv[2]);

	if (!serv.init())
		return (1);
	if (!serv.run())
		return (1);
	return (0);
}