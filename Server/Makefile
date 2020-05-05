CFLAGS= -g -ansi -Wall -pedantic -D_GNU_SOURCE 

pacman: server.o board.o utils.o sockets.o clients.o
	gcc $(CFLAGS) -o pacman server.o board.o utils.o sockets.o clients.o -lpthread

server.o: server.c server.h board.h defs.h utils.h sockets.h clients.h
	gcc $(CFLAGS) -c server.c -lpthread

board.o: board.c board.h defs.h utils.h
	gcc $(CFLAGS) -c board.c 

utils.o: utils.c utils.h defs.h
	gcc $(CFLAGS) -c utils.c

sockets.o: sockets.c sockets.h utils.h defs.h
	gcc $(CFlAGS) -c sockets.c

clients.o: clients.c clients.h utils.h defs.h
	gcc $(CFlAGS) -c clients.c


clean:
	rm -f *.o *.~ pacman *.gch
