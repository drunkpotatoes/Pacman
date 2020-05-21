/******************************************************************************
 *
 * File Name: client.c
 *
 * Authors:   Grupo 24:
 *            Inês Guedes 87202 
 * 			  Manuel Domingues 84126
 *
 * DESCRIPTION
 *		*     Implementation of the client node. Responsible for implementing
 * 			  the user interface, handle user input, and comunicating with
 * 			  the server.
 *
 *****************************************************************************/



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



                 /***************************************
                 *          GLOBAL VARIABLES            *
                 * *************************************/


/* Pacman and monster RGB */
int RGB_PAC[3];
int RGB_MON[3];

/* Pacman and monster current position */
int pacman_xy[2];
int monster_xy[2];

/* Pacman current format - [PACMAN/SUPER_PACMAN]*/
int pac_format;

/* User id on the server side*/
unsigned long my_id;

/* Enable debug variable*/
volatile int debug;

/* SDL Event to plot a piece */
Uint32 Event_ShowSomething;


int main(int argc, char** argv)
{

	int 					fd;
	pthread_t 				thread_id;

	char					ip[16];
	char 					port[6];

	struct 	sigaction 		action_sig_pipe;

	if(argc!=9)
	{
		fprintf(stderr, "Usage: pacman <server ip address> <port number> <pacman r> <pacman g> <pacman b> <monster r> <monster g> <monster b>\n");
		exit(0);
	}


	if(ip_verf(argv[1]) == 0)
	{
		strcpy(ip, argv[1]);
	}
	else 			exit(0);


	if(port_verf(argv[2]) == 0)
	{
		strcpy(port, argv[2]);
	}
	else 			exit(0);

	if(rgb_verf(argv[3], argv[4], argv[5]) == 0)
	{
		RGB_PAC[0] = atoi(argv[3]);
		RGB_PAC[1] = atoi(argv[4]);
		RGB_PAC[2] = atoi(argv[5]);
	}

	else 			exit(0);

	if(rgb_verf(argv[6], argv[7], argv[8]) == 0)
	{
		RGB_MON[0] = atoi(argv[6]);
		RGB_MON[1] = atoi(argv[7]);
		RGB_MON[2] = atoi(argv[8]);
	}

	else 			exit(0);


	pac_format = PACMAN;



   	/* sets ignore to sig pipe signal*/
   	memset(&action_sig_pipe, 0 , sizeof(struct sigaction));
	action_sig_pipe.sa_handler = SIG_IGN;
	sigemptyset(&action_sig_pipe.sa_mask);
	sigaction(SIGPIPE, &action_sig_pipe, NULL);


	debug = 0;
	
	

	/* sets up server connetion)*/
	if ( (server_setup(&fd, ip, port) ) == -1) 			server_disconnect(fd);
	
	/* creates thread to listem to server*/
	/* this thread will update the SDL event*/
	if (pthread_create(&thread_id , NULL, server_listen_thread, (void*) &fd)) 		{func_err("pthread_create"); exit(0);}
	if (pthread_detach(thread_id)) 													{func_err("pthread_detach"); exit(0);}

	

	/* game loop will send the moves to the server*/
	/* it will also print what it receives from the listen server thread*/
	if (game_loop(fd) == -1) 					server_disconnect(fd);


	close(fd);


	getchar();

	return 0;
}

/******************************************************************************
 * server_listen() 
 *
 * Arguments:
 *			void * - pointer to socket file ID.
 * Returns:
 *			void * - NULL
 * Side-Effects:
 *
 * Description: Listen to all server comunication. If it receives a piece it
 * 				Push a SQL ShowSomething Event, if it receives the score board
 * 				it will print it, if it receives a disconnect message it will
 * 				push a SQL Quit event. In advance it will also save the current
 * 				position of its pacman and monster as well as pacman format.
 *
 ******************************************************************************/
void* server_listen_thread(void * arg)
{
	
	int 						fd,n,x,y,r,g,b,piece;

	char 						buffer[BUFF_SIZE];
	char 						buffer_aux[BUFF_SIZE];

	unsigned long 				id;

	struct timeval 				tv;

	SDL_Event 					event;
	Event_ShowSomething_Data 	* event_data;


	memset(buffer, ' ', BUFF_SIZE*sizeof(char));
	memset(buffer_aux, ' ', BUFF_SIZE*sizeof(char));

	fd = *((int*) arg);


	/* sets time struct to 0 second - no timeout */
    tv.tv_sec = 0;
    tv.tv_usec = 0;

	/* sets a timeout of 0 second => removes timeout*/
	setsockopt(fd,SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

	while(1)
	{

		/* waits for server message*/
		if (( n = recv(fd, buffer, BUFF_SIZE, 0) ) == -1) 												return NULL;

		buffer[n] = '\0';

		if (n == 0) 	continue;

		debug_print("LISTEN", buffer, pthread_self(),0,debug);

		if(strstr(buffer, "PT"))
		{

			if (sscanf(buffer, "%*s %d @ %d:%d [%d,%d,%d] # %lx", &piece,&y,&x,&r,&g,&b, &id) != 7)		{inv_format(buffer); return NULL;}

			/* allocates memory for event struct */
			if ( (event_data = malloc(sizeof(Event_ShowSomething_Data)) )== NULL) 						mem_err("event_data");

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

					pac_format = PACMAN;
				}

				else if(piece == POWER_PACMAN)
				{
					pacman_xy[0] = x;
					pacman_xy[1] = y;

					pac_format = POWER_PACMAN;
				}

				else if (piece == MONSTER)
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

		else if(strstr(buffer, "SC"))
		{
			sscanf(buffer,"%*s %[^\n]", buffer_aux);
			printf("%s\n", buffer_aux);

		}
		else if(strstr(buffer, "DC"))
		{

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

/******************************************************************************
 * game_loop()
 *
 * Arguments:
 *			int    -  socket file ID.
 * Returns:
 *			int    -  returns -1 on broken connection eitheir from the server
 * 					  or from the client side - by closing the SDL window.
 * Side-Effects:
 *
 * Description: This function is responsable for the handling the user
 * 				interface and user input. It will use a SDL library queue to
 * 				pull user input (mouse and keyboard) and user interface events
 * 				pushed by the server_listen thread.
 *
 ******************************************************************************/
int game_loop(int fd)
{
	SDL_Event 	event;
	int 		n,prev_x, prev_y, board_x, board_y, window_x, window_y, piece,r,g,b;
	char 		buffer[BUFF_SIZE];

	/*monster and packman position*/
	int x = 0;
	int y = 0;

	memset(buffer, ' ', BUFF_SIZE*sizeof(char));

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

				free(data);

				if 	  (piece == EMPTY)					clear_place(x,y);		
				
				else if(piece == PACMAN)				paint_pacman(x,y,r,g,b);
			
				else if(piece == MONSTER)				paint_monster(x,y,r,g,b);	

				else if(piece == POWER_PACMAN) 			paint_powerpacman(x,y,r,g,b);

				else if(piece == LEMON)					paint_lemon(x,y);

				else if(piece == CHERRY)				paint_cherry(x,y);
		

				else /* not expected piece */ 			{inv_piece(piece); continue;}
			}

			/* pacman movement */
			if(event.type == SDL_MOUSEMOTION)
			{
				
				window_x = event.motion.x;
				window_y = event.motion.y;
				get_board_place(window_x, window_y, & board_x, &board_y);

				n = sprintf(buffer, "MV %d @ %d:%d => %d:%d\n", pac_format, pacman_xy[1], pacman_xy[0], board_y, board_x);
				buffer[n] = '\0';
				debug_print("MAIN", buffer, pthread_self(), 1, debug);

				if (send(fd,buffer, BUFF_SIZE,0) == -1)		return -1;
				
			}

			/* monster movement */
			if(event.type == SDL_KEYDOWN)
			{
				if (event.key.keysym.sym == SDLK_w )
				{
					n = sprintf(buffer, "MV %d @ %d:%d => %d:%d\n", MONSTER, monster_xy[1], monster_xy[0], monster_xy[1]-1, monster_xy[0]);
					
					buffer[n] = '\0';
					debug_print("MAIN", buffer, pthread_self(), 1, debug);

					if (send(fd,buffer, BUFF_SIZE,0) == -1)		return -1;
				}
				else if (event.key.keysym.sym == SDLK_s)
				{
					n = sprintf(buffer, "MV %d @ %d:%d => %d:%d\n", MONSTER, monster_xy[1], monster_xy[0], monster_xy[1]+1, monster_xy[0]);

					buffer[n] = '\0';
					debug_print("MAIN", buffer, pthread_self(), 1, debug);

					if (send(fd,buffer, BUFF_SIZE,0) == -1)		return -1;
				}
				else if(event.key.keysym.sym == SDLK_a)
				{
					n = sprintf(buffer, "MV %d @ %d:%d => %d:%d\n", MONSTER, monster_xy[1], monster_xy[0], monster_xy[1], monster_xy[0]-1);
					buffer[n] = '\0';
					debug_print("MAIN", buffer, pthread_self(), 1, debug);

					if (send(fd,buffer, BUFF_SIZE,0) == -1)		return -1;
				}
				else if(event.key.keysym.sym  == SDLK_d)
				{
					n = sprintf(buffer, "MV %d @ %d:%d => %d:%d\n",MONSTER, monster_xy[1], monster_xy[0], monster_xy[1], monster_xy[0]+1);
					buffer[n] = '\0';
					debug_print("MAIN", buffer, pthread_self(), 1, debug);

					if (send(fd,buffer, BUFF_SIZE,0) == -1)		return -1;
				}

			}
		}
	}
}

/******************************************************************************
 * server_setup()
 *
 * Arguments:
 *			int  * - pointer to socket file ID (empty). To be filed by this 
 * 					 function.
 * 			char * - server IP address.
 * 			char * - server port number.
 * Returns:
 *			int    -  Returns 0 on sucess and -1 if an error occured.
 * Side-Effects:
 *
 * Description: This function is responsable for establishing a connection with
 * 				the server and receive all setup information from the board.
 * 				This function will fail eitheir if the connections timesout,
 * 				the connection is shutdown on the server side, the messages
 * 				received don't follow the protocol or if any message is lost 
 * 				during the exchange. 
 *
 ******************************************************************************/

int server_setup(int * rfd, char* server_ip, char* server_port)
{

	int fd,n,row,col,nr_pieces,x,y,r,g,b,piece,nr;

	unsigned long id;
	struct addrinfo **res;
	char buffer[BUFF_SIZE];

    struct timeval tv;

    memset(buffer, ' ', BUFF_SIZE*sizeof(char));


     /* sets time struct to 1 second */
    tv.tv_sec = SECONDS_TIMEOUT;
    tv.tv_usec = USECONDS_TIMEOUT;

	if ( (res = malloc(sizeof(struct addrinfo*)) ) == NULL) 				mem_err("Addrinfo Information");


	while(1)
	{
		
		debug_print("MAIN", "Connecting to Server...", pthread_self(),2,debug);

		/* connects to server */
		if ( (fd = client_connect(res, server_ip, server_port)) == -1) 		{fprintf(stderr, "It was not possible to establishe a connection with the server\n"); exit(1);}

		*rfd = fd; 

		n = sprintf(buffer, "RQ\n");
		buffer[n] = '\0';

		/* sends a connection request to server */
		if (send(fd, buffer,BUFF_SIZE,0) == -1) 							{func_err("send"); return -1;}

		debug_print("MAIN", buffer, pthread_self(),1,debug);

		/* sets a timeout of 1 second*/
		setsockopt(fd,SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

		
		if ( (n = recv(fd,buffer,BUFF_SIZE,0) )== -1) 						{fprintf(stderr,"Connectiong Timedout...\n"); return -1;};

		buffer[n] = '\0';

		debug_print("MAIN", buffer, pthread_self(),0,debug);
		nr_pieces = 0;

		/* server welcome */
		if(strstr(buffer,"WE") != NULL)
		{	


				if(sscanf(buffer, "%*s # %lx\n", &my_id) != 1) 				{inv_format(buffer); return -1;}

				n = sprintf(buffer, "CC [%d,%d,%d] [%d,%d,%d]\n", RGB_PAC[0], RGB_PAC[1], RGB_PAC[2], RGB_MON[0], RGB_MON[1], RGB_MON[2]);

				buffer[n] = '\0';

				/* sends colour selection to server */
				if (send(fd, buffer,BUFF_SIZE,0) == -1) 					{func_err("send"); return -1;}

				debug_print("MAIN", buffer, pthread_self(),1,debug);

				/* waits for map layout */
				if (( n = recv(fd, buffer, BUFF_SIZE, 0) ) == -1) 			{fprintf(stderr,"Connectiong Timedout...\n"); return -1;};

				buffer[n] = '\0';
				
				debug_print("MAIN", buffer, pthread_self(),0,debug);

				if(strstr(buffer,"MP") == NULL)  							{inv_msg(buffer); return -1;}
	 		
				if(sscanf(buffer, "%*s %d:%d\n", &row, &col) != 2) 			{inv_format(buffer); return -1;}

				/* print board of size row x col */
				create_board_window(col, row);

				while(1)
				{
					if ((n = recv(fd, buffer, BUFF_SIZE, 0) )==-1) 			{fprintf(stderr,"Connectiong Timedout...\n"); return -1;};
					
					buffer[n] = '\0';
					debug_print("MAIN", buffer, pthread_self(),0,debug);


					/* if receives summary message terminates */
					if (strstr(buffer, "SS") != NULL ) 		break;


					/* not expected message, disconnects*/
					if ( (strstr(buffer, "PT") == NULL)  ) 					{inv_msg(buffer); return -1;}


					nr_pieces++;

					if (sscanf(buffer, "%*s %d @ %d:%d [%d,%d,%d] # %lx\n", &piece,&y,&x,&r,&g,&b, &id) != 7) 	{inv_format(buffer); return -1;}


					if     (piece == BRICK)					paint_brick(x,y);

					else if(piece == PACMAN)				paint_pacman(x,y,r,g,b);
				
					else if(piece == MONSTER)				paint_monster(x,y,r,g,b);	

					else if(piece == POWER_PACMAN) 			paint_powerpacman(x,y,r,g,b);

					else if(piece == LEMON)					paint_lemon(x,y);

					else if(piece == CHERRY)				paint_cherry(x,y);
			

					else /* not expected piece */ 			{inv_piece(piece); return -1;}


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
				if (sscanf(buffer, "%*s %d",&nr) != 1) 						{inv_format(buffer); return -1;}

				debug_print("MAIN", buffer, pthread_self(),0,debug);

				if(nr_pieces >= nr)
				{
					/* all good*/
					n = sprintf(buffer, "OK\n");
					buffer[n] = '\0';

					if (send(fd,buffer,BUFF_SIZE,0) == -1) 					{func_err("send"); return -1;}

					debug_print("MAIN", buffer, pthread_self(),1,debug);

				}

				debug_print("MAIN", "Server Setup Completed...", pthread_self(),2,debug);

				return fd;
		}

		/*server wait */
		else if (strstr(buffer,"W8") != NULL)
		{
				debug_print("MAIN", "Server is FULL. Waiting 10 and retrying...", pthread_self(),2,debug);

				/* closes connection */
				close(fd);

				/* sleeps and retries */
				sleep(10);
		}
	}
}

/******************************************************************************
 * server_setup()
 *
 * Arguments:
 *			int    - socket file ID.
 * Returns:
 *			void  
 * Side-Effects:
 *
 * Description: This function is responsable for shuting down connection with
 * 				the server. It will notify the server, close the socket and 
 * 				close the SDL window.
 *
 ******************************************************************************/

void server_disconnect(int fd)
{

	char buffer[BUFF_SIZE];
	int  n;

	memset(buffer, ' ', BUFF_SIZE*sizeof(char));

	debug_print("MAIN", "Disconnecting...", pthread_self(),2,debug);

	n = sprintf(buffer, "DC\n");
	buffer[n] = '\0';
	send(fd,buffer,BUFF_SIZE,0);
				
	debug_print("MAIN", buffer, pthread_self(),1,debug);

	close(fd);

	close_board_windows();

	exit(1);
}