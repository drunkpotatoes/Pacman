#include "defs.h"
#include "utils.h"

typedef struct board_pieces{
	int piece;
	unsigned long user_id;
	int r;
	int g;
	int b;

}board_piece;
	

typedef struct board_infos{

    board_piece** board;
    int   row;
    int   col;
    int   empty;

}board_info;


board_info init_board();
int print_board(int,int,board_piece**);
int empty_spaces(int,int,board_piece**);
void place_piece(board_piece **,int,int,int,unsigned long,int,int,int);
char* print_piece(board_piece **, int, int, char*);
void free_board(int row, board_piece**);
int is_empty(int, int, board_piece**);
int is_pacman(int, int, board_piece**);
int is_monster(int, int, board_piece**);
int is_brick(int, int, board_piece**);
int piece_is_correct(int,int,int,unsigned long,board_piece**);




