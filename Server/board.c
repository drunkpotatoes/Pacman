#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "board.h"

int print_board(int row, int col, board_piece** board)
{
	int i, j;


	for (i = 0; i<row; i++)
	{
		for(j=0; j<col;j++)
		{
			
			printf("%c",board[i][j].piece);
		}

		printf("\n");
	}

	return 0;
}

board_info init_board()
{
	FILE* fid;
	int row, col;
	int i, j, c;
	board_piece ** board;

	board_info bi;


	if (  (fid = fopen(BOARD_FILE, "r") ) == NULL)
	{
		fprintf(stderr, "Error opening the board file %s...\nExiting...\n", BOARD_FILE);
		exit(1);
	}

	

	if ( fscanf(fid,"%d %d", &col , &row) != 2 )
	{
		fprintf(stderr, "Error with the format of the board file %s...\nExiting...\n", BOARD_FILE);
		exit(1);
	}

	
	if ( (board = (board_piece**) malloc (row*sizeof(board_piece*))  ) == NULL )					mem_err("board");

	for(i = 0 ; i < row ; i++)
	{

		if ( ( board[i] = (board_piece*) malloc (col*sizeof(board_piece))  ) == NULL )				mem_err("board");
	

		for (j = 0; j < col ; j++)
		{

			c = fgetc(fid);

			if (c == '\n')
			{
				c = fgetc(fid);
			}
			board[i][j].piece = c;
			board[i][j].user_id = 0;
			board[i][j].r = 0;
			board[i][j].g = 0;
			board[i][j].b = 0;

		}
	}

	bi.board = board;
	bi.row = row;
	bi.col = col;

	bi.empty = empty_spaces(row,col,board);

	bi.max_fruits =0;
	bi.cur_fruits =0;

	return bi;
}


int empty_spaces(int row, int col, board_piece** board)
{
	int i,j;
	int empty = 0;

	for (i = 0; i<row; i++)
	{
		for(j=0; j<col;j++)
		{
			
			if (board[i][j].piece == EMPTY) empty++;
		}
	}


	return empty;
}

char* print_piece(board_piece ** board, int row, int col, char* buffer)
{
	int n;

	n = sprintf(buffer, "%d @ %d:%d [%d,%d,%d] # %lx", board[row][col].piece ,row ,col ,board[row][col].r, board[row][col].g, board[row][col].b, board[row][col].user_id);
	
	buffer[n] = '\0';

	return buffer;
}


void place_piece(board_piece ** board, int piece, int row, int col, unsigned long id, int r, int g, int b)
{

	board[row][col].piece = piece;
	board[row][col].user_id = id;
	board[row][col].r = r;
	board[row][col].g = g;
	board[row][col].b = b;

	return;
}


void switch_pieces(board_piece** board, int row1, int row2, int col1, int col2)
{
	board_piece aux;

	aux = board[row1][col1];
	board[row1][col1] = board[row2][col2];
	board[row2][col2] = aux;

	return; 
}


void clear_board_place(board_piece ** board, int row, int col)
{
	board[row][col].piece = EMPTY;
	board[row][col].user_id = 0;
	board[row][col].r = 0;
	board[row][col].g = 0;
	board[row][col].b = 0;
}

void clear_player(board_piece** board, int rows, int cols, unsigned long id, int* coord)
{
	int i, j;
	int count = 0;

	for (i = 0; i < rows ; i++)
	{
		for(j = 0; j < cols ; j++)
		{
			if(board[i][j].user_id == id && (board[i][j].piece == PACMAN || board[i][j].piece == MONSTER || board[i][j].piece == POWER_PACMAN))
			{
				coord[count*2] = i;
				coord[count*2+1] = j;
				clear_board_place(board, i, j);

				count++;
			}

			if (count == 2) return;
		}
	}

	return;
}

void free_board(int row, board_piece** board)
{
	int i;

	for (i = 0; i < row; i++)
	{
		free(board[i]);
	}

	free(board);

	return;
}


int is_empty(int row, int col, board_piece** board)
{
	if(board[row][col].piece == EMPTY)
		return 1;
	else
		return 0;
}

int is_pacman(int row, int col, board_piece** board)
{
	if(board[row][col].piece == PACMAN)
		return 1;
	else
		return 0;
}

int is_monster(int row, int col, board_piece** board)
{
	if(board[row][col].piece == MONSTER)
		return 1;
	else
		return 0;
}

int is_brick(int row, int col, board_piece** board)
{
	if(board[row][col].piece == BRICK)
		return 1;
	else
		return 0;
}

int is_any_fruit(int row, int col, board_piece** board)
{
	if( (board[row][col].piece == LEMON ) || (board[row][col].piece == CHERRY))
		return 1;
	else
		return 0;
}

unsigned long int get_id(board_piece ** board, int row, int col)
{
	return board[row][col].user_id;
}

int piece_is_correct(int row, int col, int piece, unsigned long  id, board_piece** board)
{
	if ( (board[row][col].piece == piece) && (board[row][col].user_id == id) )
		return 1;
	else
		return 0;
}


void move_and_clear(board_piece** board, int row, int col, int to_row, int to_col)
{
	board_piece aux;

	aux = board[row][col];

	board[to_row][to_col] = aux;

	clear_board_place(board, row, col);

	return;

}

void move (board_piece** board, int row, int col, int to_row, int to_col)
{
	board_piece aux;

	aux = board[row][col];

	board[to_row][to_col] = aux;

	return;
}

