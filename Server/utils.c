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


void func_err(char* variable)
{
	fprintf (stderr, "\nError in function %s...\n", variable);
}

void inv_msg()
{
	fprintf(stderr, "\nInvalid Message Received...\n");
}

