
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -fsanitize=address -g3
NAME = ircserv
SRCS = Channel.cpp Client.cpp ClientInfo.cpp \
		Eventq.cpp main.cpp Operator.cpp \
		Server.cpp Worker.cpp
INCLUDE_DIR = ./include
BONUS_DIR = ./IRCbot
OBJS = $(SRCS:.cpp=.o)
RM = rm -f

all: $(NAME)

debug:
	$(MAKE) fclean
	$(MAKE) DEBUG=1 all

bonus:
	$(MAKE) -C$(BONUS_DIR) all

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

clean:
	$(MAKE) -C$(BONUS_DIR) clean
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(BONUS_DIR)/ircbot
	$(RM) $(NAME)

re: 
	$(MAKE) fclean
	$(MAKE) all

.PHONY : all clean fclean re debug bonus