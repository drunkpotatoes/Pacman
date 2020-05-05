#include "defs.h"
#include "utils.h"

typedef struct board_pieces{
	int piece;
	int user_id;
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
int print_board(int, int , board_piece**);
int empty_spaces(int,int,board_piece**);




