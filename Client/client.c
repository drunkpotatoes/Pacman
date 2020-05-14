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


int pacman_xy[2];
int monster_xy[2];


unsigned long my_id;


Uint32 Event_ShowSomething;

void* server_listen_thread(void * arg)
{
	
	int fd,n,x,y,r,g,b,piece;

	char buffer[BUFF_SIZE];

	unsigned long id;

	struct timeval tv;

	SDL_Event event;
	Event_ShowSomething_Data * event_data;

	fd = *((int*) arg);


	/* sets time struct to 0 second - no timeout */
    tv.tv_sec = 0;
    tv.tv_usec = 0;

	/* sets a timeout of 0 second => removes timeout*/
	setsockopt(fd,SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

	while(1)
	{

		/* waits for server message*/
		if (( n = recv(fd, buffer, BUFF_SIZE, 0) ) == -1) 										return NULL;

		buffer[n] = '\0';

		if (n == 0) 	continue;

		printf("Received : %s\n",buffer);

		if (strstr(buffer, "OK") != NULL)
		{
			if (sscanf(buffer,"%*s %d %d:%d",&piece,&y, &x)  != 3 ) 				{inv_format(buffer); return NULL;}

			if (piece == PACMAN)
			{
				pacman_xy[0] = x;
				pacman_xy[1] = y;
			}

			if (piece == MONSTER)
			{
				monster_xy[0] = x;
				monster_xy[1] = y;
			}
		}

		else if(strstr(buffer, "PT"))
		{

			if (sscanf(buffer, "%*s %d @ %d:%d [%d,%d,%d] # %lx", &piece,&y,&x,&r,&g,&b, &id) != 7)		{inv_format(buffer); return NULL;}

			/* allocates memory for event struct */
			if ( (event_data = malloc(sizeof(Event_ShowSomething_Data)) )== NULL) 					mem_err("event_data");

			/* assigns values*/
			event_data->x = x;	event_data->y = y;	event_data->piece = piece;	event_data->r = r;	event_data->g = g;	event_data->b = b;

			/* clears event data */
			SDL_memset(&event, 0, sizeof(event)); /* or SDL_zero(event) */

			/* defines event type */
			event.type = Event_ShowSomething;

			/* assigns the event data */
			event.user.data1 = event_data;

			/* sends the event */
			SDL_PushEvent(&event);



			if(id == my_id)
			{
				if (piece == PACMAN)
				{
					pacman_xy[0] = x;
					pacman_xy[1] = y;
				}

				if (piece == MONSTER)
				{
					monster_xy[0] = x;
					monster_xy[1] = y;
				}
			}

		}

		else if(strstr(buffer,"CL"))
		{
			/* allocates memory for event struct */
			if ( (event_data = malloc(sizeof(Event_ShowSomething_Data)) ) == NULL ) 	mem_err("even_data");

			if (sscanf(buffer, "%*s %d:%d", &y,&x) != 2) 								{inv_format(buffer); return NULL;}

			/* assigns values*/
			event_data->x = x;	event_data->y = y;	event_data->piece = EMPTY;	event_data->r = 0;	event_data->g = 0;	event_data->b = 0;

			/* clears event data */
			SDL_memset(&event, 0, sizeof(event)); /* or SDL_zero(event) */

			/* defines event type */
			event.type = Event_ShowSomething;

			/* assigns the event data */
			event.user.data1 = event_data;

			/* sends the event */
			SDL_PushEvent(&event);

		}
		else if(strstr(buffer, "DC"))
		{
			fprintf(stdout , "Server SHUTDOWN\n");

			/* clears event data */
			SDL_memset(&event, 0, sizeof(event)); /* or SDL_zero(event) */

			/* defines event type */
			event.type = SDL_QUIT;

			/* sends the event */
			SDL_PushEvent(&event);

			/* returns */
			return NULL;
				
		}

	}
}


int main(int argc, char** argv)
{

	int fd;
	
	pthread_t 	thread_id;

	RGB_PAC[0] = 255;
	RGB_PAC[1] = 0;
	RGB_PAC[2] = 0;

	RGB_MON[0] = 0;
	RGB_MON[1] = 0;
	RGB_MON[2] = 255;

	

	/* sets up server connetion)*/
	if ( (server_setup(&fd) ) == -1) 			server_disconnect(fd);
	
	/* creates thread to listem to server*/
	/* this thread will update the SDL event*/
	pthread_create(&thread_id , NULL, server_listen_thread, (void*) &fd);

	

	/* game loop will send the moves to the server*/
	/* it will also print what it receives from the listen server thread*/
	if (game_loop(fd) == -1) 					server_disconnect(fd);


	getchar();

	return 0;
}

int game_loop(int fd)
{
	SDL_Event 	event;
	int 		n,prev_x, prev_y, board_x, board_y, window_x, window_y, piece,r,g,b;
	char 		buffer[BUFF_SIZE];



	/*monster and packman position*/
	int x = 0;
	int y = 0;

	Event_ShowSomething =  SDL_RegisterEvents(1);

	SDL_GetMouseState(&prev_x, &prev_y);

	while (1)
	{
		while (SDL_PollEvent(&event)) 
		{
			/* user closed window */
			if(event.type == SDL_QUIT) 					return -1;
			

			/* display piece */
			if(event.type == Event_ShowSomething)
			{
				/* we get the data (created with the malloc) */
				Event_ShowSomething_Data * data = event.user.data1;

				/* retrieve data */
				x = data->x; y = data->y; r = data->r; g = data->g; b = data->b; piece = data->piece;


				if 	  (piece == EMPTY)					clear_place(x,y);		
				
				else if(piece == PACMAN)				paint_pacman(x,y,r,g,b);
			
				else if(piece == MONSTER)				paint_monster(x,y,r,g,b);	

				else if(piece == POWER_PACMAN) 			paint_powerpacman(x,y,r,g,b);

				else if(piece == BRICK)					paint_brick(x,y);
		

				else /* not expected piece */ 			{fprintf(stderr,"\nInvalid Piece Received...\n"); return -1;}
			}

			/* pacman movement */
			if(event.type == SDL_MOUSEMOTION)
			{
				
				window_x = event.motion.x;
				window_y = event.motion.y;
				get_board_place(window_x, window_y, & board_x, &board_y);

				n = sprintf(buffer, "MV %d @ %d:%d => %d:%d", PACMAN, pacman_xy[1], pacman_xy[0], board_y, board_x);
				buffer[n] = '\0';
				printf("Sent: %s\n", buffer);

				if (send(fd,buffer, BUFF_SIZE,0) == -1)		return -1;
				
			}

			/* monster movement */
			if(event.type == SDL_KEYDOWN)
			{
				if (event.key.keysym.sym == SDLK_w )
				{
					n = sprintf(buffer, "MV %d @ %d:%d => %d:%d", MONSTER, monster_xy[1], monster_xy[0], monster_xy[1]-1, monster_xy[0]);
					
					buffer[n] = '\0';
					printf("Sent: %s\n", buffer);

					if (send(fd,buffer, BUFF_SIZE,0) == -1)		return -1;
				}
				else if (event.key.keysym.sym == SDLK_s)
				{
					n = sprintf(buffer, "MV %d @ %d:%d => %d:%d", MONSTER, monster_xy[1], monster_xy[0], monster_xy[1]+1, monster_xy[0]);

					buffer[n] = '\0';
					printf("Sent: %s\n", buffer);

					if (send(fd,buffer, BUFF_SIZE,0) == -1)		return -1;
				}
				else if(event.key.keysym.sym == SDLK_a)
				{
					n = sprintf(buffer, "MV %d @ %d:%d => %d:%d", MONSTER, monster_xy[1], monster_xy[0], monster_xy[1], monster_xy[0]-1);
					buffer[n] = '\0';
					printf("Sent: %s\n", buffer);

					if (send(fd,buffer, BUFF_SIZE,0) == -1)		return -1;
				}
				else if(event.key.keysym.sym  == SDLK_d)
				{
					n = sprintf(buffer, "MV %d @ %d:%d => %d:%d",MONSTER, monster_xy[1], monster_xy[0], monster_xy[1], monster_xy[0]+1);
					buffer[n] = '\0';
					printf("Sent: %s\n", buffer);

					if (send(fd,buffer, BUFF_SIZE,0) == -1)		return -1;
				}

			}
		}
	}

}


int server_setup(int * rfd)
{

	int fd,n,row,col,nr_pieces,x,y,r,g,b,piece,nr;

	unsigned long id;
	struct addrinfo **res;
	char buffer[BUFF_SIZE];

    struct timeval tv;


     /* sets time struct to 1 second */
    tv.tv_sec = SECONDS_TIMEOUT;
    tv.tv_usec = USECONDS_TIMEOUT;

	if ( (res = malloc(sizeof(struct addrinfo*)) ) == NULL) 	mem_err("Addrinfo Information");

	while (1)
	{
		
		/* connects to server */
		if ( (fd = client_connect(res, SERVER_IP, SERVER_PORT)) == -1) 		{fprintf(stderr, "It was not possible to establish a connection with the server\n"); exit(1);}

		*rfd = fd; 

		n = sprintf(buffer, "RQ\n");

		buffer[n] = '\0';

		

		/* sends a connection request to server */
		if (send(fd, buffer,BUFF_SIZE,0) == -1) 				{func_err("send"); return -1;}

		printf("Sent: %s\n", buffer);
		/* sets a timeout of 1 second*/
		setsockopt(fd,SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

		if ( (n = recv(fd,buffer,BUFF_SIZE,0) )== -1) 			{func_err("recv"); return -1;};

		buffer[n] = '\0';
		printf("Received: %s\n", buffer);

		nr_pieces = 0;

		/* server welcome */
		if(strstr(buffer,"WE") != NULL)
		{	


			if(sscanf(buffer, "%*s # %lx\n", &my_id) != 1) 		{inv_format(buffer); return -1;}

			n = sprintf(buffer, "CC [%d,%d,%d] [%d,%d,%d]\n", RGB_PAC[0], RGB_PAC[1], RGB_PAC[2], RGB_MON[0], RGB_MON[1], RGB_MON[2]);

			buffer[n] = '\0';

			/* sends colour selection to server */
			if (send(fd, buffer,BUFF_SIZE,0) == -1) 			{func_err("send"); return -1;}

			printf("Sent: %s\n", buffer);

			/* waits for map layout */
			if (( n = recv(fd, buffer, BUFF_SIZE, 0) ) == -1) 	{func_err("recv"); return -1;}


			buffer[n] = '\0';
			

			printf("Received: %s\n", buffer);


			if(strstr(buffer,"MP") == NULL)  					{inv_msg(buffer); return -1;}
 		
			if(sscanf(buffer, "%*s %d:%d\n", &row, &col) != 2) 	{inv_format(buffer); return -1;}

			/* print board of size row x col */
			create_board_window(row, col);

			while(1)
			{
				if ((n = recv(fd, buffer, BUFF_SIZE, 0) )==-1) 	{func_err("recv"); return -1;}
				
				buffer[n] = '\0';
				printf("Received %s\n", buffer);


				/* if receives summary message terminates */
				if (strstr(buffer, "SS") != NULL ) 		break;


				/* not expected message, disconnects*/
				if ( (strstr(buffer, "PT") == NULL)  ) 			{inv_msg(buffer); return -1;}


				nr_pieces++;

				if (sscanf(buffer, "%*s %d @ %d:%d [%d,%d,%d] # %lx\n", &piece,&y,&x,&r,&g,&b, &id) != 7) 	{inv_format(buffer); return -1;}


				if     (piece == BRICK)					paint_brick(x,y);

				else if(piece == PACMAN)				paint_pacman(x,y,r,g,b);
			
				else if(piece == MONSTER)				paint_monster(x,y,r,g,b);	

				else if(piece == POWER_PACMAN) 			paint_powerpacman(x,y,r,g,b);
		

				else /* not expected piece */ 			{fprintf(stderr,"\nInvalid Piece Received...\n"); return -1;}


				if(id == my_id)
				{
					if (piece == PACMAN)
					{
						pacman_xy[0] = x;
						pacman_xy[1] = y;
					}

					if (piece == MONSTER)
					{
						monster_xy[0] = x;
						monster_xy[1] = y;
					}
				}

			}


			/* gets number of messages sent */
			if (sscanf(buffer, "%*s %d",&nr) != 1) 		{inv_format(buffer); return -1;}

			printf("Received: %s\n", buffer);

			if(nr_pieces >= nr)
			{
				/* all good*/
				n = sprintf(buffer, "OK");
				buffer[n] = '\0';

				if (send(fd,buffer,BUFF_SIZE,0) == -1) 	{func_err("send"); return -1;}

				printf("Sent: %s\n", buffer);

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


void server_disconnect(int fd)
{

	char buffer[BUFF_SIZE];
	int  n;
	n = sprintf(buffer, "DC");
	buffer[n] = '\0';

	send(fd,buffer,BUFF_SIZE,0);
				
	printf("Sent: %s\n", buffer);

	close(fd);

	close_board_windows();

	exit(1);
}