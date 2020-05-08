#include "defs.h"
#include "board.h"
#include "utils.h"
#include "sockets.h"
#include "clients.h"

void *	client_thread		(void*);
void *	accept_thread 		(void*);

int	client_setup		(int, int*);
int	client_loop		(int, int*);
void 	client_disconnect  	(int, int*);

void 	write_play_to_main	(char*);

int 	pacman_movement		(int, int, int, int, int*, client*);
int 	monster_movement	(int, int, int, int, int*, client*);
