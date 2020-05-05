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

	
	RGB_PAC[0] = 255;
	RGB_PAC[1] = 0;
	RGB_PAC[2] = 0;

	RGB_MON[0] = 0;
	RGB_MON[1] = 0;
	RGB_MON[2] = 255;

	server_setup();

	getchar();

	return 0;
}


int server_setup()
{
	int fd,n,row,col,nr_pieces,x,y,r,g,b,piece,nr;
	struct addrinfo **res;
	char buffer[100];

    struct timeval tv;

    SDL_Event event;


     /* sets time struct to 1 second */
    tv.tv_sec =1;
    tv.tv_usec = 0;

	res = malloc(sizeof(struct addrinfo*));

	while (1)
	{
		
		/* connects to server */
		if ( (fd = client_connect(res, SERVER_IP, SERVER_PORT)) == -1) 		return -1;

		n = sprintf(buffer, "RQ\n");

		buffer[n] = '\0';

		/* sends a connection request to server */
		send(fd, buffer,BUFF_SIZE,0);

		/* sets a timeout of 1 second*/
		setsockopt(fd,SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

		n = recv(fd,buffer,BUFF_SIZE,0);

		buffer[n] = '\0';

		nr_pieces = 0;

		/* server welcome */
		if(strstr(buffer,"WE") != NULL)
		{	

			n = sprintf(buffer, "CC %d,%d,%d %d,%d,%d\n", RGB_PAC[0], RGB_PAC[1], RGB_PAC[2], RGB_MON[0], RGB_MON[1], RGB_MON[2]);

			buffer[n] = '\0';

			/* sends colour selection to server */
			send(fd, buffer,BUFF_SIZE,0);


			/* waits for map layout */
			n = recv(fd, buffer, BUFF_SIZE, 0);
			buffer[n] = '\0';

			if(strstr(buffer,"MP") == NULL)  return -1;
 		
			sscanf(buffer, "%*s %d:%d\n", &row, &col);

			/* print board of size row x col */
			create_board_window(row, col);

			while(1)
			{
				n = recv(fd, buffer, BUFF_SIZE, 0);
				buffer[n] = '\0';
				printf("%s\n", buffer);


				/* if receives summary message terminates */
				if (strstr(buffer, "SS") != NULL ) 		break;


				/* not expected message, disconnects*/
				if ( (strstr(buffer, "PT") == NULL)  )
				{
					n = sprintf(buffer, "DC");
					buffer[n] = '\0';
					send(fd,buffer,BUFF_SIZE,0);
					printf("%s\n", buffer);

					return -1;
				}


				

				nr_pieces++;

				sscanf(buffer, "%*s %d@%d:%d %d,%d,%d", &piece,&y,&x,&r,&g,&b);


				if     (piece == BRICK)					paint_brick(x,y);

				else if(piece == PACMAN)				paint_pacman(x,y,r,g,b);
			
				else if(piece == MONSTER)				paint_monster(x,y,r,g,b);	

				else if(piece == POWER_PACMAN) 			paint_powerpacman(x,y,r,g,b);
		

				else /* not expected piece */
				{
					/* disonnects */
					n = sprintf(buffer, "DC");
					buffer[n] = '\0';
					send(fd,buffer,BUFF_SIZE,0);

					return -1;
				}

			}


			/* gets number of messages sent */
			sscanf(buffer, "%*s %d",&nr);

			if(nr_pieces == nr)
			{
				/* all good*/
				n = sprintf(buffer, "OK");
				buffer[n] = '\0';
				send(fd,buffer,BUFF_SIZE,0);
				printf("%s\n", buffer);

			}

			return 0;


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