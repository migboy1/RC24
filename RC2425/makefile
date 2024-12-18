CC     = g++
CFLAGS = -Wall -std=c++11

.PHONY: all clean

# Main targets
all: player GS

# Player executable
player: client/client.cpp utils.o
	$(CC) $(CFLAGS) -o player client/client.cpp utils.o

# Server executable
GS: server/server.cpp utils.o 
	$(CC) $(CFLAGS) -o GS server/server.cpp utils.o

# Shared utilities
utils.o: utils.cpp utils.hpp const.hpp
	$(CC) $(CFLAGS) -c utils.cpp -o utils.o

clean:
	rm -f player GS *.o
	rm -r SCORES
	rm -r GAMES
