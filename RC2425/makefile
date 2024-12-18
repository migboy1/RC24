CC     = g++
# -Wall  - this flag is used to turn on most compiler warnings
CFLAGS = -Wall

.PHONY: all clean

all: user server

user: client/client.cpp utils.o
	$(CC) $(CFLAGS) -o player client/client.cpp utils.o

server: server/server.cpp utils.o
	$(CC) $(CFLAGS) -o DS server/server.cpp utils.o

utils.o: utils.cpp const.hpp
	$(CC) $(CFLAGS) -c -o utils.o utils.cpp

	
clean:
	rm -f player *.o
