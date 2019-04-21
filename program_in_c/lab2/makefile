# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g -Wall -o


# the build target executable:
SERVER = ftps
CLIENT = ftpc

all: $(SERVER) $(CLIENT)

$(SERVER):	$(SERVER).c
	$(CC) $(CFLAGS) $(SERVER) $(SERVER).c

$(CLIENT): $(CLIENT).c
	$(CC) $(CFLAGS) $(CLIENT) $(CLIENT).c

clean:
	$(RM) -rf $(CLIENT) $(CLIENT).dSYM $(SERVER) $(SERVER).dSYM
