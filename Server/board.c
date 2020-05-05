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