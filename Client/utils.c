/******************************************************************************
 *
 * File Name: utils.c
 *
 * Authors:   Grupo 24:
 *            Inês Guedes 87202 
 * 			  Manuel Domingues 84126
 *
 * DESCRIPTION
 *		*     Contains miscellaneous functions used by the program.
 * 			  mostly default error handling.
 *
 *****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

/******************************************************************************
 * mem_err()
 *
 * Arguments:
 *			 char* 	 - name of the variable
 * Returns:
 *			 
 * Side-Effects: Stops program execution
 *
 * Description:  Used after a error allocation memory with malloc.
 * 				 Prints the message in stderr.
 * 				 Exits the program.
 *
 ******************************************************************************/
void mem_err (char * variable)
{
  
  	fprintf (stderr, "\nError allocating memory for %s...\n", variable);
  	fprintf (stderr, "Exiting program...\n");
  	exit(1);
}

/******************************************************************************
 * func_err()
 *
 * Arguments:
 *			 char* 	 - name of the function
 * Returns:
 *			 
 * Side-Effects:
 *
 * Description:  Used after a error received from a given function
 * 				 Prints the message in stderr.
 *
 ******************************************************************************/
void func_err(char* func_name)
{
	fprintf (stderr, "\nError in function: %s...\n", func_name);
}

/******************************************************************************
 * inv_msg()
 *
 * Arguments:
 *			 char* 	 - message received
 * Returns:
 *			 
 * Side-Effects: 
 *
 * Description:  Notifies the user by printing and warning with the message 
 * 				 received contaning the invalid message to stderr. Happens
 * 				 when the message is not knowed by the protocol.
 *
 ******************************************************************************/
void inv_msg(char* msg)
{
	fprintf(stderr, "\nInvalid Message Received: %s\n", msg);
}


/******************************************************************************
 * inv_format()
 *
 * Arguments:
 *			 char* 	 - message received
 * Returns:
 *			 
 * Side-Effects: 
 *
 * Description:  Notifies the user by printing and warning with the message 
 * 				 received contaning the invalid format to stderr. Happens
 * 				 when the format doesn't follow the protocol.
 *
 ******************************************************************************/
void inv_format(char* msg)
{
	fprintf(stderr, "\nInvalid Formated Message Received: %s\n", msg);
}

/******************************************************************************
 * inv_piece()
 *
 * Arguments:
 *			 int 	- piece received
 * Returns:
 *			 
 * Side-Effects: 
 *
 * Description:  Notifies the user by printing a warning with the piece
 * 				 received to stderr.
 *
 ******************************************************************************/
void inv_piece(int piece)
{
	fprintf(stderr, "\nInvalid Piece Received : '%c' \n", piece);
}

/******************************************************************************
 * debug_print()
 *
 * Arguments:
 *			 char* 			- custom name of the module calling the function
 * 			 char* 			- message
 * 			 unsigned long  - id of the module
 * 			 int 			- mode flag (0 - Received a message)
 * 										(1 - Sent a message)
 * 										(2 - Display information)
 * 			 debug 			- flag saying if debug mode is on.
 *
 * Returns:
 *			 
 * Side-Effects: 
 *
 * Description:  Tests if debug is active (inside the function for cleaness
 *				 porpuses), if so, prints the debug message.
 *
 ******************************************************************************/
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

/******************************************************************************
 * ip_verf()
 *
 * Arguments:
 *			 char*  	- IP string
 * Returns:
 *			 int  		- Returns 0 if the IP is correct, -1 if not.
 * Side-Effects: 
 *
 * Description:  Checks if the IP is correct.
 *
 ******************************************************************************/
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

/******************************************************************************
 * port_verf()
 *
 * Arguments:
 *			 char*  	- port string
 * Returns:
 *			 int  		- Returns 0 if the port is correct, -1 if not.
 * Side-Effects: 
 *
 * Description:  Checks if the port is correct.
 *
 ******************************************************************************/
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

/******************************************************************************
 * ip_verf()
 *
 * Arguments:
 *			 char*  	- string contaning r value
 * 			 char*      - string contaning g value
 * 			 char*      - string contaning b value
 * Returns:
 *			 int  		- Returns 0 if the RGB is correct, -1 if not.
 * Side-Effects: 
 *
 * Description:  Checks if the RGB is correct.
 *
 ******************************************************************************/

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