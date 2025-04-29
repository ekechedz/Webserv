NAME = webserv
CXX = c++
CXXFLAGS = -Wall -Werror -Wextra -g -std=c++98

SRC_DIR = src
SRCS = \
	$(SRC_DIR)/main.cpp \
	$(SRC_DIR)/ConfigParser.cpp \
	$(SRC_DIR)/LocationConfig.cpp \
	$(SRC_DIR)/ServerConfig.cpp \
	$(SRC_DIR)/utils.cpp \
	$(SRC_DIR)/Server.cpp \
	$(SRC_DIR)/Request.cpp \

OBJS = $(SRCS:.cpp=.o)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

all: $(NAME)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
