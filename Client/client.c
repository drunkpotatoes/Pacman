#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <signal.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <pthread.h>


#include "client.h"


int RGB_PAC[3];
int RGB_MON[3];


int main(int argc, char** argv)
{

	int fd, n;

	char buffer[BUFF_SIZE];
	
	RGB_PAC[0] = 255;
	RGB_PAC[1] = 0;
	RGB_PAC[2] = 0;

	RGB_MON[0] = 0;
	RGB_MON[1] = 0;
	RGB_MON[2] = 255;

	

	/* sets up server connetion)*/
	if ( (fd = server_setup() ) == -1) 	
	{
		n = sprintf(buffer, "DC");
		buffer[n] = '\0';

		if (send(fd,buffer,BUFF_SIZE,0) == -1) 	{func_err("send"); return -1;}
				
		printf("%s\n", buffer);

		exit(1);

		close(fd);
	}

	/* creates thread to listem to server*/
	/* this thread is responsable to print things on the board*/
	

	/* creates thread to send to the server*/
	/* this threads is the one responsable to get keyboard moves*/
	getchar();

	return 0;
}


int server_setup()
{

	int fd,n,row,col,nr_pieces,x,y,r,g,b,piece,nr;
	struct addrinfo **res;
	char buffer[BUFF_SIZE];

    struct timeval tv;

    SDL_Event event;


     /* sets time struct to 1 second */
    tv.tv_sec = SECONDS_TIMEOUT;
    tv.tv_usec = USECONDS_TIMEOUT;

	if ( (res = malloc(sizeof(struct addrinfo*)) ) == NULL) 	mem_err("Addrinfo Information");

	while (1)
	{
		
		/* connects to server */
		if ( (fd = client_connect(res, SERVER_IP, SERVER_PORT)) == -1) 		return -1;

		n = sprintf(buffer, "RQ\n");

		buffer[n] = '\0';

		printf("%s\n", buffer);

		/* sends a connection request to server */
		if (send(fd, buffer,BUFF_SIZE,0) == -1) 				{func_err("send"); return -1;}

		/* sets a timeout of 1 second*/
		setsockopt(fd,SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

		if ( (n = recv(fd,buffer,BUFF_SIZE,0) )== -1) 			{func_err("recv"); return -1;};

		buffer[n] = '\0';
		printf("%s\n", buffer);

		nr_pieces = 0;

		/* server welcome */
		if(strstr(buffer,"WE") != NULL)
		{	

			n = sprintf(buffer, "CC [%d,%d,%d] [%d,%d,%d]\n", RGB_PAC[0], RGB_PAC[1], RGB_PAC[2], RGB_MON[0], RGB_MON[1], RGB_MON[2]);

			buffer[n] = '\0';

			printf("%s\n", buffer);

	

			/* sends colour selection to server */
			if (send(fd, buffer,BUFF_SIZE,0) == -1) 			{func_err("send"); return -1;}


			/* waits for map layout */
			if (( n = recv(fd, buffer, BUFF_SIZE, 0) ) == -1) 	{func_err("recv"); return -1;}

			printf("n = %d\n", n);

			buffer[n] = '\0';
			

			printf("%s\n", buffer);


			if(strstr(buffer,"MP") == NULL)  					{inv_msg(); return -1;}
 		
			if(sscanf(buffer, "%*s %d:%d\n", &row, &col) != 2) 	{func_err("sscanf"); return -1;}

			/* print board of size row x col */
			create_board_window(row, col);

			while(1)
			{
				if ((n = recv(fd, buffer, BUFF_SIZE, 0) )==-1) 	{func_err("recv"); return -1;}
				
				buffer[n] = '\0';
				printf("%s\n", buffer);


				/* if receives summary message terminates */
				if (strstr(buffer, "SS") != NULL ) 		break;


				/* not expected message, disconnects*/
				if ( (strstr(buffer, "PT") == NULL)  ) 			{inv_msg(); return -1;}


				

				nr_pieces++;
				printf("%s\n", buffer);

				if (sscanf(buffer, "%*s %d @ %d:%d [%d,%d,%d]", &piece,&y,&x,&r,&g,&b) != 6) 	{func_err("sscanf"); return -1;}


				if     (piece == BRICK)					paint_brick(x,y);

				else if(piece == PACMAN)				paint_pacman(x,y,r,g,b);
			
				else if(piece == MONSTER)				paint_monster(x,y,r,g,b);	

				else if(piece == POWER_PACMAN) 			paint_powerpacman(x,y,r,g,b);
		

				else /* not expected piece */ 			{fprintf(stderr,"\nInvalid Piece Received...\n"); return -1;}

			}


			/* gets number of messages sent */
			if (sscanf(buffer, "%*s %d",&nr) != 1) 		{func_err("sscanf"); return -1;}

			printf("%s\n", buffer);

			if(nr_pieces >= nr)
			{
				/* all good*/
				n = sprintf(buffer, "OK");
				buffer[n] = '\0';

				if (send(fd,buffer,BUFF_SIZE,0) == -1) 	{func_err("send"); return -1;}

				printf("%s\n", buffer);

			}

			return fd;


		}

		/*server wait */
		else if (strstr(buffer,"W8") != NULL)
		{
			printf("%s\n", buffer);

			/* closes connection */
			close(fd);

			/* sleeps and retries */
			sleep(10);
		}
		

		/* timeout  - waits 1s*/
		sleep(1);

	}
}