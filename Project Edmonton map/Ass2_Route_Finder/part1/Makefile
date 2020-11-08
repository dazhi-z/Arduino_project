CC=g++
OBJS=dijkstra.o server.o digraph.o
CFLAGS=-c -Wall -O2
LFLAGS=-static
PROGRAM=server

server: server.o dijkstra.o digraph.o
	$(CC) server.o dijkstra.o digraph.o -o server $(LFLAGS)

server.o: server.cpp
	$(CC) server.cpp -o server.o $(CFLAGS)

dijkstra.o: dijkstra.cpp
	$(CC) dijkstra.cpp -o dijkstra.o $(CFLAGS)

digraph.o: digraph.cpp
	$(CC) digraph.cpp -o digraph.o $(CFLAGS)

clean:
	@rm -f $(OBJS)
	@rm -f $(PROGRAM)