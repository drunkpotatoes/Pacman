#include "defs.h"
#include "utils.h"
#include "sockets.h"
#include "clients.h"
#include "board.h"


void *	client_thread		(void*);
void *	accept_thread 		(void*);

int	client_setup		(int, int*);
int	client_loop		(int, int);
void 	client_disconnect  	(int, int);


int 	main_thread 		();
int 	write_play_to_main	(char*, int);

int 	pacman_movement		(board_piece**, int, int, int, int, int, int, int, int, int*);
int 	monster_movement	(board_piece**, int, int, int, int, int, int, int, int, int*);

int bounce(board_piece**,int,int,int,int,int,int,int*,int*);
