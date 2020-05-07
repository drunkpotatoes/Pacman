#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

	

	if ( fscanf(fid,"%d %d", &row , &col) != 2 )
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
			board[i][j].r = 0;
			board[i][j].g = 0;
			board[i][j].b = 0;

		}
	}

	print_board(row, col, board);


	bi.board = board;
	bi.row = row;
	bi.col = col;

	bi.empty = empty_spaces(row,col,board);


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

	n = sprintf(buffer, "%d @ %d:%d [%d,%d,%d]", board[row][col].piece ,row ,col ,board[row][col].r, board[row][col].g, board[row][col].b);
	
	buffer[n] = '\0';

	return buffer;
}


void place_piece(board_piece ** board, int piece, int row, int col, unsigned long id, int r, int g, int b)
{


	/* needs write protection*/
	board[row][col].piece = piece;
	board[row][col].user_id = id;
	board[row][col].r = r;
	board[row][col].g = g;
	board[row][col].b = b;

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

int piece_is_correct(int row, int col, int piece, unsigned long  id, board_piece** board)
{
	if ( (board[row][col].piece == piece) && (board[row][col].user_id == id) )
		return 1;
	else
		return 0;
}

