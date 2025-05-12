NAME = webserv
CXX = c++

CXXFLAGS = -Wall -Werror -Wextra -g -std=c++98 -pedantic-errors

SRC_DIR = src
OBJ_DIR = obj

SRCS = \
	$(SRC_DIR)/main.cpp \
	$(SRC_DIR)/ConfigParser.cpp \
	$(SRC_DIR)/LocationConfig.cpp \
	$(SRC_DIR)/ServerConfig.cpp \
	$(SRC_DIR)/Utils.cpp \
	$(SRC_DIR)/Server.cpp \
	$(SRC_DIR)/Request.cpp \
	$(SRC_DIR)/Response.cpp \
	$(SRC_DIR)/Socket.cpp \

OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

all: $(NAME)

# Pattern rule for building .o files into obj/
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)
	rm -f www/post_output.txt

re: fclean all

.PHONY: all clean fclean re
