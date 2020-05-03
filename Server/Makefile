CFLAGS= -g -ansi -Wall -pedantic -D_GNU_SOURCE

pacman: server.o board.o utils.o
	gcc $(CFLAGS) -o pacman server.o board.o utils.o -lm

server.o: server.c server.h board.h defs.h utils.h
	gcc $(CFLAGS) -c server.c 

board.o: board.c board.h defs.h utils.h
	gcc $(CFLAGS) -c board.c 

utils.o: utils.c utils.h defs.h
	gcc $(CFLAGS) -c utils.c



clean:
	rm -f *.o *.~ pacman *.gch
