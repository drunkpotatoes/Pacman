#include "defs.h"
#include "utils.h"

typedef struct _board_info
{
    int** board;
    int   row;
    int   col;
    int   empty;

} board_info;

board_info init_board();
int print_board(int, int , int**);
int empty_spaces(int,int,int**);

