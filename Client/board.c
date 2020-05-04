#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "board.h"

int print_board(int row, int col, int** board)
{
	int i, j;


	for (i = 0; i<row; i++)
	{
		for(j=0; j<col;j++)
		{
			
			printf("%c",board[i][j]);
		}

		printf("\n");
	}

	return 0;
}

int** init_board()
{
	FILE* fid;
	int row, col;
	int i, j, c;
	int ** board;


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

	
	if ( (board = (int**) malloc (row*sizeof(int*))  ) == NULL )					mem_err("board");

	for(i = 0 ; i < row ; i++)
	{

		if ( ( board[i] = (int*) malloc (col*sizeof(int))  ) == NULL )				mem_err("board");
	

		for (j = 0; j < col ; j++)
		{

			c = fgetc(fid);

			if (c == '\n')
			{
				c = fgetc(fid);
			}
			board[i][j] = c;
		}
	}

	print_board(row, col, board);

	return board;

}