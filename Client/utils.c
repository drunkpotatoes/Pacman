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


void debug_print(char*name , char* message, unsigned long id , int flag, int debug)
{

	if (debug)
	{/* mode 0 - received , mode 1 - sent , mode 2 - info*/ 
		if (flag == 0)
		{
			printf("%-6s THREAD (%lx) [<-]: %s\n", name, id, message);
		}
		else if (flag == 1)
		{
			printf("%-6s THREAD (%lx) [->]: %s\n", name, id, message);
		}
		else if(flag == 2)
		{	
			printf("%-6s THREAD (%lx) [!!]: %s\n\n", name, id, message);
		}
	}
}


int ip_verf(char *st)
{
    char aux[4][5],str[5];
    int i=0,n=0;

    if (sscanf(st,"%[^.].%[^.].%[^.].%s",aux[1],aux[2],aux[3],aux[4]) != 4) /*splits the ip into 3, 3 digit numbers */
        return -1;

    for(i=1;i<=4;i++)
    {
        sscanf(aux[i],"%d",&n);

        /*out of expected range*/ 
        if ((n>999) || (n<0))				return -1;
        
        sprintf(str,"%d",n);

        if (strcmp(aux[i],str) != 0)		return -1;
    }
    return 0;
}

int port_verf(char *st)
{
    int 	n;
    char 	str[6];

    n = 0;

    sscanf(st,"%d",&n);

    /*checks if port number is valid, port numbers below 1024 are already reserved*/
    if (n < 1024)   			return -1;

    sprintf(str,"%d",n);

    if (strcmp(st,str) != 0)	return -1;

    return 0;
}

int rgb_verf(char* cr, char* cg, char* cb)
{
	int r,g,b;

	r = atoi(cr);
	g = atoi(cg);
	b = atoi(cb);

	if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255)
	{
		return -1;
	}

	return 0;
}