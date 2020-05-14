#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

void mem_err (char * variable)
{
  
  	fprintf (stderr, "\nError allocating memory for %s...\n", variable);
  	fprintf (stderr, "Exiting program...\n");
  	exit(1);
}


void func_err(char* func_name)
{
	fprintf (stderr, "\nError in function: %s...\n", func_name);
}

void inv_msg(char* msg)
{
	fprintf(stderr, "\nInvalid Message Received: %s\n", msg);
}


void inv_format(char* msg)
{
	fprintf(stderr, "\nInvalid Formated Message Received: %s\n", msg);
}

void inv_piece(int piece)
{
	fprintf(stderr, "\nInvalid Piece Received : '%c' \n", piece);
}

