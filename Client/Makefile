CFLAGS= -g -ansi -Wall -pedantic -D_GNU_SOURCE

pacman: client.o board.o utils.o sockets.o
	gcc $(CFLAGS) -o pacman client.o board.o utils.o sockets.o -lm

client.o: client.c client.h board.h defs.h utils.h sockets.h
	gcc $(CFLAGS) -c client.c 

board.o: board.c board.h defs.h utils.h
	gcc $(CFLAGS) -c board.c 

utils.o: utils.c utils.h defs.h
	gcc $(CFLAGS) -c utils.c

sockets.o: sockets.c sockets.h utils.h defs.h
	gcc $(CFlAGS) -c sockets.c


clean:
	rm -f *.o *.~ pacman *.gch
