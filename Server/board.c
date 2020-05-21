/******************************************************************************
 *
 * File Name: board.c
 *
 * Authors:   Grupo 24:
 *            Inês Guedes 87202 
 * 			  Manuel Domingues 84126
 *
 * DESCRIPTION
 *		*     Contains all the functions that interact with the game's board.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "board.h"

/******************************************************************************
 * print_board() 
 *
 * Arguments:
 *			int 	     - number of rows of the board
 * 			int 		 - number of columns of the board
 * 			board_pice** - board struct pointer
 * Returns:
 *			void
 * Side-Effects:
 *
 * Description: Prints the board on stdout
 *
 ******************************************************************************/

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

/******************************************************************************
 * init_board() 
 *
 * Arguments:
 *			void
 * Returns:
 *			board_info 	 - board info structure filled with init values
 * Side-Effects:
 *
 * Description: Reads the file contaning board information and initializes
 * 				the board as well as some information abou the board.
 *
 ******************************************************************************/

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
			board[i][j].counter = 0;

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

/******************************************************************************
 * empty_spaces() 
 *
 * Arguments:
 *			int 	     - number of rows of the board
 * 			int 		 - number of columns of the board
 * 			board_pice** - board struct pointer
 * Returns:
 *			int 		 - number of empty pieces
 * Side-Effects:
 *
 * Description: Counts the number of empty pieces in the board.
 *
 ******************************************************************************/


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

/******************************************************************************
 * print_piece() 
 *
 * Arguments:
 * 			board_pice** - board struct pointer
 *			int 	     - number of rows of the board
 * 			int 		 - number of columns of the board
 * 			char * 		 - buffer to save the return variable
 * Returns:
 *			char * 		 - received buffer filled with piece information
 * Side-Effects:
 *
 * Description: Prints a default message contaning the piece information
 * 				of the piece in the given column and row to a buffer.
 *
 ******************************************************************************/
char* print_piece(board_piece ** board, int row, int col, char* buffer)
{
	int n;

	n = sprintf(buffer, "%d @ %d:%d [%d,%d,%d] # %lx", board[row][col].piece ,row ,col ,board[row][col].r, board[row][col].g, board[row][col].b, board[row][col].user_id);
	
	buffer[n] = '\0';

	return buffer;
}

/******************************************************************************
 * place_piece() 
 *
 * Arguments:
 * 			board_pice**  - board struct pointer
 * 			int 		  - piece 
 *			int 	      - x coordenate
 * 			int 		  - y coordenate
 * 			unsigned long - id of the owner of the piece
 * 			int 		  - rgb r of the piece
 * 			int 		  - rgb g of the piece
 * 			int 		  - rgb b of the piece 
 * Returns:
 *			void
 * Side-Effects:
 *
 * Description: Places a piece on the board,
 *
 ******************************************************************************/

void place_piece(board_piece ** board, int piece, int row, int col, unsigned long id, int r, int g, int b)
{

	board[row][col].piece = piece;
	board[row][col].user_id = id;
	board[row][col].r = r;
	board[row][col].g = g;
	board[row][col].b = b;

	return;
}


/******************************************************************************
 * switch_pieces() 
 *
 * Arguments:
 * 			board_pice**  - board struct pointer
 *			int 	      - x coordenate of piece 1
 * 			int 		  - y coordenate of piece 1
 *			int 	      - x coordenate of piece 2
 * 			int 		  - y coordenate of piece 2
 * Returns:
 *			void
 * Side-Effects:
 *
 * Description: Switches the pieces located on the coordenates received.
 *
 ******************************************************************************/
void switch_pieces(board_piece** board, int row1, int row2, int col1, int col2)
{
	board_piece aux;

	aux = board[row1][col1];
	board[row1][col1] = board[row2][col2];
	board[row2][col2] = aux;

	return; 
}

/******************************************************************************
 * clear_board_place() 
 *
 * Arguments:
 * 			board_pice**  - board struct pointer
 *			int 	      - x coordenate 
 * 			int 		  - y coordenate 
 * Returns:
 *			void
 * Side-Effects:
 *
 * Description: Clears the position received
 *
 ******************************************************************************/
void clear_board_place(board_piece ** board, int row, int col)
{
	board[row][col].piece = EMPTY;
	board[row][col].user_id = 0;
	board[row][col].r = 0;
	board[row][col].g = 0;
	board[row][col].b = 0;
}

/******************************************************************************
 * clear_player() 
 *
 * Arguments:
 * 			board_pice**  - board struct pointer
 *			int 	      - board number of rows
 * 			int 		  - board number of columns
 *			unsigned long - id of the player
 * 			int* 		  - coordenates of the players pieces(empty). To be 
 * 							filled by this functions.
 * Returns:
 *			void
 * Side-Effects:
 *
 * Description: Searches the whole board for the pieces of the player
 * 				with matching id. When it finds them, saves the coordenates
 * 				in the input pointer and clears the board position.
 *
 ******************************************************************************/
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

/******************************************************************************
 * free_board() 
 *
 * Arguments:
 * 			int 		  - number of rows
 * 			board_pice**  - board struct pointer
 * Returns:
 *			void
 * Side-Effects:
 *
 * Description: Frees the board memory.
 *
 ******************************************************************************/
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

/******************************************************************************
 *  is_empty() 
 *
 * Arguments:
 * 			int 		  - x coordenate
 * 			int 		  - y coordenate
 * 			board_pice**  - board struct pointer
 * Returns:
 *			int 		  - Returns 1 if is empty and 0 otherwise.
 * Side-Effects:
 *
 * Description: Checks if a position is empty.
 *
 ******************************************************************************/
int is_empty(int row, int col, board_piece** board)
{
	if(board[row][col].piece == EMPTY)
		return 1;
	else
		return 0;
}

/******************************************************************************
 *  is_pacman() 
 *
 * Arguments:
 * 			int 		  - x coordenate
 * 			int 		  - y coordenate
 * 			board_pice**  - board struct pointer
 * Returns:
 *			int 		  - Returns 1 if has a pacman and 0 otherwise.
 * Side-Effects:
 *
 * Description: Checks if a position has a pacman.
 *
 ******************************************************************************/
int is_pacman(int row, int col, board_piece** board)
{
	if(board[row][col].piece == PACMAN)
		return 1;
	else
		return 0;
}

/******************************************************************************
 *  is_power_pacman() 
 *
 * Arguments:
 * 			int 		  - x coordenate
 * 			int 		  - y coordenate
 * 			board_pice**  - board struct pointer
 * Returns:
 *			int 		  - Returns 1 if has a power pacman and 0 otherwise.
 * Side-Effects:
 *
 * Description: Checks if a position has a power pacman.
 *
 ******************************************************************************/
int is_power_pacman(int row, int col, board_piece** board)
{
	if(board[row][col].piece == POWER_PACMAN)
		return 1;
	else
		return 0;
}

/******************************************************************************
 *  is_monster() 
 *
 * Arguments:
 * 			int 		  - x coordenate
 * 			int 		  - y coordenate
 * 			board_pice**  - board struct pointer
 * Returns:
 *			int 		  - Returns 1 if has a monster and 0 otherwise.
 * Side-Effects:
 *
 * Description: Checks if a position has a monster.
 *
 ******************************************************************************/
int is_monster(int row, int col, board_piece** board)
{
	if(board[row][col].piece == MONSTER)
		return 1;
	else
		return 0;
}

/******************************************************************************
 *  is_brick() 
 *
 * Arguments:
 * 			int 		  - x coordenate
 * 			int 		  - y coordenate
 * 			board_pice**  - board struct pointer
 * Returns:
 *			int 		  - Returns 1 if has a brick and 0 otherwise.
 * Side-Effects:
 *
 * Description: Checks if a position has a brick.
 *
 ******************************************************************************/
int is_brick(int row, int col, board_piece** board)
{
	if(board[row][col].piece == BRICK)
		return 1;
	else
		return 0;
}

/******************************************************************************
 *  is_any_fruit() 
 *
 * Arguments:
 * 			int 		  - x coordenate
 * 			int 		  - y coordenate
 * 			board_pice**  - board struct pointer
 * Returns:
 *			int 		  - Returns 1 if has a fruit and 0 otherwise.
 * Side-Effects:
 *
 * Description: Checks if a position has a fruit.
 *
 ******************************************************************************/
int is_any_fruit(int row, int col, board_piece** board)
{
	if( (board[row][col].piece == LEMON ) || (board[row][col].piece == CHERRY))
		return 1;
	else
		return 0;
}

/******************************************************************************
 * get_id() 
 *
 * Arguments:
 * 			int 		  - x coordenate
 * 			int 		  - y coordenate
 * 			board_pice**  - board struct pointer
 * Returns:
 *			unsigned long - id of the owner of the piece
 * Side-Effects:
 *
 * Description: Gets the id of the owner of the piece in the received position.
 *
 ******************************************************************************/
unsigned long int get_id(board_piece ** board, int row, int col)
{
	return board[row][col].user_id;
}

/******************************************************************************
 * piece_is_correct() 
 *
 * Arguments:
 * 			int 		  - x coordenate
 * 			int 		  - y coordenate
 *  		int 		  - piece
 * 			unsigned long - id
 * 			board_pice**  - board struct poin
 * Returns:
 *			int 		  - Returns 1 if it is correct, 0 otherwise.
 * Side-Effects:
 *
 * Description: Checks if a piece is correct acording to the board.
 *
 ******************************************************************************/
int piece_is_correct(int row, int col, int piece, unsigned long  id, board_piece** board)
{
	if ( (board[row][col].piece == piece) && (board[row][col].user_id == id) )
		return 1;
	else
		return 0;
}

/******************************************************************************
 * move_and_clear() 
 *
 * Arguments:
 *			board_pice**  - board struct pointer
 * 			int 		  - previous x coordenate
 * 			int 		  - previous y coordenate
 * 			int 		  - destination x coordenate
 * 			int 		  - destination y coordenate
 * 			
 * Returns:
 *			void
 * Side-Effects:
 *
 * Description: Moves a piece in the board, clears the place the piece was in
 * 				before.
 *
 ******************************************************************************/
void move_and_clear(board_piece** board, int row, int col, int to_row, int to_col)
{
	board_piece aux;

	aux = board[row][col];

	board[to_row][to_col] = aux;

	clear_board_place(board, row, col);

	return;

}

/******************************************************************************
 * move() 
 *
 * Arguments:
 *			board_pice**  - board struct pointer
 * 			int 		  - previous x coordenate
 * 			int 		  - previous y coordenate
 * 			int 		  - destination x coordenate
 * 			int 		  - destination y coordenate
 * 			
 * Returns:
 *			void
 * Side-Effects:
 *
 * Description: Moves a piece in the board.
 ******************************************************************************/
void move (board_piece** board, int row, int col, int to_row, int to_col)
{
	board_piece aux;

	aux = board[row][col];

	board[to_row][to_col] = aux;

	return;
}

/******************************************************************************
 * transform_pacman() 
 *
 * Arguments:
 *			board_pice**  - board struct pointer
 * 			int 		  - x coordenate
 * 			int 		  - y coordenate
 * 			
 * Returns:
 *			void
 * Side-Effects:
 *
 * Description: Upgrades pacman to super pacman.
 *
 ******************************************************************************/
void transform_pacman(board_piece** board, int row, int col, int counter)
{
	/* safety check */
	if (is_pacman(row,col,board))
	{
		board[row][col].piece = POWER_PACMAN;
		board[row][col].counter = counter;
	}
}

/******************************************************************************
 * reverse_pacman() 
 *
 * Arguments:
 *			board_pice**  - board struct pointer
 * 			int 		  - x coordenate
 * 			int 		  - y coordenate
 * 			
 * Returns:
 *			void
 * Side-Effects:
 *
 * Description: Downgrades pacman to normal pacman.
 *
 ******************************************************************************/
void reverse_pacman(board_piece** board, int row, int col)
{
	/* safety check */
	if(is_power_pacman(row,col,board))
	{
		board[row][col].piece = PACMAN;
		return;
	}
}


/******************************************************************************
 * decrement_counter() 
 *
 * Arguments:
 *			board_pice**  - board struct pointer
 * 			int 		  - x coordenate
 * 			int 		  - y coordenate
 * 			
 * Returns:
 *			int 		  - counter current number if super pacman
 * 							otherwise -1.
 * Side-Effects:
 *
 * Description: Decrements piece counter. Used to regulate pacman/power pacman
 * 				transformation.
 *
 ******************************************************************************/
int decrement_counter(board_piece** board, int row, int col)
{
	/* safety check */
	if(is_power_pacman(row,col,board))
	{
		
		return --board[row][col].counter;
	}

	return -1;
}

/******************************************************************************
 * decrement_counter() 
 *
 * Arguments:
 *			board_pice**  - board struct pointer
 * 			int 		  - x coordenate
 * 			int 		  - y coordenate
 * 			
 * Returns:
 *			int 		  - counter current number if super pacman
 * 							otherwise -1.
 * Side-Effects:
 *
 * Description: Uncrements piece counter. Used to regulate pacman/power pacman
 * 				transformation.
 *
 ******************************************************************************/
int increment_counter(board_piece** board, int row, int col)
{
	/* safety check */
	if(is_power_pacman(row,col,board))
	{
		return ++board[row][col].counter;
	}

	return -1;
}


