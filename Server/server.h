#include "defs.h"
#include "utils.h"
#include "sockets.h"
#include "clients.h"
#include "board.h"
#include "UI_library.h"

void *	client_thread		(void*);
void *	accept_thread 		(void*);
void * 	fruity_thread 		(void*);
void * 	score_thread 		(void*);

int	client_setup		(int, int*,int*);
int	client_loop		(int, int ,int);
void 	client_disconnect  	(int, int ,int);

void 	server_disconnect	();
int 	main_thread 		();
int 	write_play_to_main	(char*, int);

int 	write_fruit 		(int,int,int);

int 	pacman_movement		(board_piece**, int, int, int, int, int, int, 					int, int, int*);

int 	power_pacman_movement	(board_piece**, int, int, int, int, int, int, 					int, int, int*);

int 	monster_movement	(board_piece**, int, int, int, int, int, int, 					int, int, int*);

int 	bounce			(board_piece**,int,int,int,int,int,int,
				int*,int*);

void 	place_random_position	(board_piece**, int, int, int, int*, int);

void 	player_eats_player	(board_piece**, int, int, int, int, int, int, int, int);

