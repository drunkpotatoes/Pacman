/******************************************************************************
 *
 * File Name: server.c
 *
 * Authors:   Grupo 24:
 *            Inês Guedes 87202 
 * 			  Manuel Domingues 84126
 *
 * DESCRIPTION
 *		*     Implementation of the server node. Responsible for implementing
 * 			  the user interface, execute all the game functionalities and 
 * 			  handle client interaction.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#include "server.h"


                 /***************************************
                 *          GLOBAL VARIABLES            *
                 * *************************************/


/* list to the head of the clients list */
client* clients_head;


/* struct with the board information */
board_info status;

/*
typedef struct board_infos{

    board_piece** board;
    int   row;
    int   col;
    int   empty;
    int   max_fruits;
    int   cur_fruits;

}board_info;
*/

/* locks for the status and client structures */

pthread_mutex_t * lock_col; 			/* locks with the column index - one mutex per column*/

pthread_mutex_t   lock_empty;			/* locks board_info.empty field */

pthread_mutex_t   lock_fruits;  		/* locks board_info.max_fruits field */

pthread_mutex_t   lock_cur;				/* locks board_info.cur_fruits field */

pthread_rwlock_t  lock_clients;			/* locks access to client list in a read/write fashion */

pthread_mutex_t   lock_success; 		/* locks conditional variable shutdown success */


/* flag containing shutdown succes from all client threads */
volatile int shut_down_success; 		

/* debug flag */
volatile int debug;

/* shutdown flag */
volatile int shut_down;


/* SIGINT handler for server shutdown */
void handler (int sigtype)
{
	shut_down = 1;
}


int main(int argc, char** argv)
{

	int 						i,x,y,fid;
    pthread_t 					accept_thread_id, fruits_thread_id, score_thread_id;
   	struct 		sigaction 		action_sig_pipe;
   	struct 		sigaction 		action_int;


   	/* sets ignore to sig pipe signal*/
   	memset(&action_sig_pipe, 0 , sizeof(struct sigaction));
	action_sig_pipe.sa_handler = SIG_IGN;
	sigemptyset(&action_sig_pipe.sa_mask);
	sigaction(SIGPIPE, &action_sig_pipe, NULL);

	/* handles sig int and exits safelly*/
	memset(&action_int, 0 , sizeof(struct sigaction));
	action_int.sa_handler = handler;
	sigemptyset(&action_int.sa_mask);
	sigaction(SIGINT, &action_int, NULL);

	shut_down = 0;
	shut_down_success = 0;
	debug = 0;
	clients_head = NULL;
	status = init_board();


	/* creates board window*/
	create_board_window(status.col, status.row);

	/* places bricks */
	for (x = 0; x < status.row ; x++)
		for(y=0; y < status.col; y++)
			if(is_brick(x,y,status.board))
				paint_brick(y,x);


	/* creates lock for each col*/
	if ( (lock_col = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t)*status.col) ) == NULL) 			  mem_err("lock_col");

	/* initializes read-write lock for clients*/
	if (pthread_rwlock_init(&lock_clients,NULL) != 0)								{func_err("pthread_rwlock_init"); exit(1);}

	/* initializes lock for empty places*/
	if (pthread_mutex_init(&lock_empty,NULL) != 0)									{func_err("pthread_mutex_init"); exit(1);}

	/* initializes lock for max fruits*/
	if (pthread_mutex_init(&lock_fruits,NULL) != 0)									{func_err("pthread_mutex_init"); exit(1);}

	/* initializes lock for each col*/
	for (i = 0; i < status.col ; i++)
		if (pthread_mutex_init(&lock_col[i],NULL) != 0)								{func_err("pthread_mutex_init"); exit(1);}

	/* initializes lock for number of curr fruits*/
	if(pthread_mutex_init(&lock_cur,NULL) != 0)										{func_err("pthread_mutex_init"); exit(1);}	

	/* initializes lock for shutdown success*/
	if(pthread_mutex_init(&lock_success,NULL) != 0)									{func_err("pthread_mutex_init"); exit(1);}					

	/* creates thread to accept clients*/
	if (pthread_create(&accept_thread_id , NULL, accept_thread, (void*) &fid)) 		{func_err("pthread_create"); exit(1);}
	if (pthread_detach(accept_thread_id))											{func_err("pthread_detach"); exit(1);}

	/* creates thread to handle fruits*/
	if (pthread_create(&fruits_thread_id , NULL, fruity_thread, NULL)) 				{func_err("pthread_create"); exit(1);}
	if (pthread_detach(fruits_thread_id))											{func_err("pthread_detach"); exit(1);}

	/* creates thread to handle score*/
	if (pthread_create(&score_thread_id , NULL, score_thread, NULL)) 				{func_err("pthread_create"); exit(1);}
	if (pthread_detach(score_thread_id))											{func_err("pthread_detach"); exit(1);}

	/* thread to send information to all clients*/
	main_thread();

	/* closes accept thread socket*/
	close(fid);

	/*cleans up*/
	server_disconnect();

	exit(0);
}

                 /***************************************
                 *              THREADS                 *
                 * *************************************/

/******************************************************************************
 * main_thread() 
 *
 * Arguments:
 *			void 
 * Returns:
 *			int  - Returns 0 if it detects a shudown, -1 on error.
 * Side-Effects:
 *
 * Description: Reads the plays sent from all the threads, plots them on the 
 * 				server side SDL window and sends them to all clients.
 *
 ******************************************************************************/
int main_thread()
{


	int 	n, fd, x, y, r, g, b, piece;
	char 	buffer[BUFF_SIZE];

	/* tries to open the fifo */
	if ( (fd = open(FIFO_FILE, O_RDONLY) ) == -1)
	{
		/* if it fails to open, tries to create */
		if (mkfifo(FIFO_FILE, 0666)!=0)					{func_err("mkfifo"); return -1; }

		/* tries to open again */
		if ( (fd = open(FIFO_FILE, O_RDONLY) ) == -1)	{func_err("open");   return -1; }
	}

	debug_print("MAIN" ,"Opened Plays FIFO for Read", pthread_self(),2,debug);
	

	/* main cycle */
	while(!shut_down)
	{

		if (  (n = read(fd, buffer, BUFF_SIZE) ) == -1 ) 				continue; 

		if (n == 0)														continue;


		buffer[n] = '\0';

		debug_print("MAIN" ,buffer, pthread_self(),0,debug);

		pthread_rwlock_rdlock(&lock_clients);
		send_all_clients(clients_head,buffer,BUFF_SIZE);
		pthread_rwlock_unlock(&lock_clients);

		if(strstr(buffer, "PT"))
		{
			

			if (sscanf(buffer, "%*s %d @ %d:%d [%d,%d,%d] #", &piece,&y,&x,&r,&g,&b) != 6)		{inv_format(buffer); continue;}
			
				
				if     (piece == PACMAN)				paint_pacman(x,y,r,g,b);
			
				else if(piece == MONSTER)				paint_monster(x,y,r,g,b);	

				else if(piece == POWER_PACMAN) 			paint_powerpacman(x,y,r,g,b);

				else if(piece == CHERRY)				paint_cherry(x,y);

				else if(piece == LEMON)					paint_lemon(x,y);

				else /* not expected piece */ 			{inv_piece(piece); continue;}
		
		}

		else if(strstr(buffer,"CL"))
		{
			
			if (sscanf(buffer, "%*s %d:%d", &y,&x) != 2) 								{inv_format(buffer); continue;}
			clear_place(x,y);	
		 }	

	}

	return 0;
}

/******************************************************************************
 * fruity_thread() 
 *
 * Arguments:
 *			void *
 * Returns:
 *			void *   - NULL
 * Side-Effects:
 *
 * Description: Thread responsible for handling all the fruit spawns. It reads
 * 				all client threads requests, checks if the max number of fruits
 * 				is already on board, if not, it will calculate the time diference
 * 				from the request made and the time it reads the request. It will
 * 				subtract this value to the fruit spawn time and sleep for that
 * 				amount of time, releasing the processor.
 *
 ******************************************************************************/

void * fruity_thread (void* arg)
{
	int 				fd, ff_fd, n,lemon;
	char 				buffer[BUFF_SIZE];
	int 				rgb[3];
	long 				dif;
	struct 	timeval 	now, start;
	struct 	timespec 	dif_tv;
	
	memset(buffer,' ', BUFF_SIZE*sizeof(char));
	memset(rgb,0,3*sizeof(int));


	/* tries to opens fifo */
	if ((ff_fd = open(FIFO_FILE, O_WRONLY))== -1) 		
	{
		/*if it fails to open, tries to create */
		if (mkfifo(FIFO_FILE, 0666)!=0)							{func_err("mkfifo"); return NULL; }

		/* tries to open again */
		if ((ff_fd = open(FIFO_FILE, O_WRONLY))== -1)			{func_err("open");   return NULL; }
	}

	debug_print("FRUIT" ,"Opened Plays FIFO for Write", pthread_self(),2,debug);

	/* tries to open the fifo */
	if ( (fd = open(FRUIT_FIFO_FILE, O_RDONLY) ) == -1)
	{
		/* if it fails to open, tries to create */
		if (mkfifo(FRUIT_FIFO_FILE, 0666)!=0)					{func_err("mkfifo"); return NULL; }

		/* tries to open again */
		if ( (fd = open(FRUIT_FIFO_FILE, O_RDONLY) ) == -1)		{func_err("open");   return NULL; }
	}

	debug_print("FRUIT" ,"Opened Fruits FIFO for Read", pthread_self(),2,debug);


	while(!shut_down)
	{
		if (  (n = read(fd, buffer, BUFF_SIZE) ) == -1 ) 				continue; 

		if (n == 0)														continue;

		debug_print("FRUIT" ,buffer, pthread_self(),0,debug);

		if (strstr(buffer,"TM") != NULL)
		{
			if (sscanf(buffer, "%*s %ld %ld\n", &start.tv_sec , &start.tv_usec) != 2) 	{inv_format(buffer); continue;}
			
			/* if curr fruits are already the maxed value, it will ignore fruit spawn requests*/

			pthread_mutex_lock(&lock_fruits);
			pthread_mutex_lock(&lock_cur);

			if (status.cur_fruits >= status.max_fruits) 								
			{
				pthread_mutex_unlock(&lock_fruits);
				pthread_mutex_unlock(&lock_cur); 
				continue;
			}

			pthread_mutex_unlock(&lock_cur);
			pthread_mutex_unlock(&lock_fruits);

			/* gets current time*/
			gettimeofday(&now,NULL);

			/* calculates time passed after receiving message*/
			dif =  (now.tv_usec - start.tv_usec) + (now.tv_sec - start.tv_sec)*1000000;

			/* receveid before spawn time - should be the usual case*/
			if (dif < FRUIT_ST_USECONDS)
			{
				dif_tv.tv_sec = floor((FRUIT_ST_USECONDS - dif)/1000000);
				dif_tv.tv_nsec = (FRUIT_ST_USECONDS - dif)%1000000;
				
				/* sleeps through the remaining time to spawn the fruit and relases the processor*/
				nanosleep(&dif_tv,NULL);
			}

			/* gets random fruit*/
			srand(time(NULL));
			lemon = rand()%2;

			if(lemon)
			{
				place_random_position(status.board, status.row, status.col,LEMON, rgb, ff_fd);
			}
			else
			{
				place_random_position(status.board, status.row, status.col,CHERRY, rgb, ff_fd);
			}

			pthread_mutex_lock(&lock_cur);
			status.cur_fruits++;
			pthread_mutex_unlock(&lock_cur);

		}

		else 													{inv_msg(buffer); continue;}
	}

	close(fd);
	close(ff_fd);

	return NULL;
}

/******************************************************************************
 * score_thread() 
 *
 * Arguments:
 *			void *
 * Returns:
 *			void *   - NULL
 * Side-Effects:
 *
 * Description: Thread responsible for sending all clients the score. It will
 * 				be sleeping for most of the time while waiting for the score
 * 				to be sent again. It will also release the processor when 
 * 				sleeping.
 *
 ******************************************************************************/
void * score_thread (void* arg)
{
	

	int  				max_number_clients,i,n;
	char ** 			buffer;
	char				buffer_aux[BUFF_SIZE];
	struct timespec 	time;

	memset(buffer_aux, ' ', sizeof(char)*BUFF_SIZE);

	time.tv_sec = SCORE_TIME_SECONDS;
	time.tv_nsec = 0;

	n = 0;

	/* calculates max number of clients */
	pthread_mutex_lock(&lock_empty);
	pthread_rwlock_rdlock(&lock_clients);

	max_number_clients =  number_of_clients(clients_head) + status.empty/4;

	pthread_rwlock_unlock(&lock_clients);
	pthread_mutex_unlock(&lock_empty);
	


	/* inits buffer allocation*/
	if ((buffer = (char**) malloc(sizeof(char*)*max_number_clients)) == NULL) 			mem_err("score buffer");

	for (i = 0; i < max_number_clients ; i++)
	{
		if ((buffer[i] = (char*) malloc(sizeof(char)*BUFF_SIZE) ) == NULL ) 			mem_err("score buffer");
		memset(buffer[i], ' ', sizeof(char)*BUFF_SIZE);
		buffer[i][BUFF_SIZE-1] = '\0';
	}

	while(!shut_down)
	{
		nanosleep(&time,NULL);

		pthread_rwlock_rdlock(&lock_clients);
		n = print_score_buffer(clients_head, buffer);

		i = sprintf(buffer_aux,"SC ===================SCORE BOARD======================\n");
		buffer_aux[i] = '\0';
		send_all_clients(clients_head, buffer_aux,BUFF_SIZE);
		debug_print("SCORE", buffer_aux, pthread_self(), 1, debug);

		for (i = 0; i < n; i++)
		{
			send_all_clients(clients_head, buffer[i],BUFF_SIZE);
			debug_print("SCORE", buffer[i], pthread_self(), 1, debug);
		}

		i = sprintf(buffer_aux,"SC ====================================================\n");
		buffer_aux[i] = '\0';
		send_all_clients(clients_head, buffer_aux,BUFF_SIZE);
		debug_print("SCORE", buffer_aux, pthread_self(), 1, debug);

		pthread_rwlock_unlock(&lock_clients);
	}


	/* frees previous buffer */
	for (i = 0; i < max_number_clients ; i++)
	{
		free(buffer[i]);
	}

	free(buffer);


	return NULL;
}

/******************************************************************************
 * accept_thread() 
 *
 * Arguments:
 *			void *   - pointer to server socket (empty). To be filled by this
 * 					   thread in order for the main thread to be able to close
 * 					   the socket.
 * Returns:
 *			void *   - NULL
 * Side-Effects:
 *
 * Description: Thread responsible for waiting for new client connections.
 * 				It will actively wait for a connection request and then create
 * 				a client thread to handle the new client.
 ******************************************************************************/
void * accept_thread (void* arg)
{
	
	int 					fd,newfd,n;

	int 					*p_fd;

	char 					buffer[BUFF_SIZE];

	struct sockaddr_in 		addr;
	struct addrinfo 		**res;
	struct timeval 			tv;

	socklen_t 				addrlen;
    
    pthread_t 				thread_id;

    p_fd = (int*) arg;

    memset(buffer, ' ', sizeof(char)*BUFF_SIZE);

    addrlen = sizeof(struct sockaddr);
    /* sets time struct to 1 second */
    tv.tv_sec  = SECONDS_TIMEOUT;
    tv.tv_usec = USECONDS_TIMEOUT;



    /* allocs memory for struct */
	if ( (res = malloc(sizeof(struct addrinfo*)) ) == NULL ) 		mem_err("Addrinfo Struct");

	if ( (fd = server_open(res, SERVER_PORT) )== -1)
	{
		exit(1);
	}

	*p_fd = fd;

	
	debug_print("ACCEPT", "Accepting Connections from Clients...", pthread_self(), 2, debug);

	while(!shut_down)
	{
		/* accepts connection */
		newfd=accept(fd,(struct sockaddr*)&addr,&addrlen);


		/* sets a timeout of 1 second*/
		setsockopt(newfd,SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

		/* gets message from client*/
		n = recv(newfd, buffer,BUFF_SIZE,0);

		/* cleans buffer */
		buffer[n] = '\0';

		debug_print("ACCEPT" ,buffer, pthread_self(),0,debug);

		/* connection request*/	
		if(strstr(buffer, "RQ") != NULL)
		{
			/* creates thread to handle client */
			if (pthread_create(&thread_id , NULL, client_thread, (void*) &newfd)) 	func_err("pthread_create");
			if (pthread_detach(thread_id)) 											func_err("pthread_detach");

		}
	}

	/* closes connection */
	close_connection(res,fd);


	return NULL;
}

/******************************************************************************
 * client_thread() 
 *
 * Arguments:
 *			void *   - pointer to socket ID file.
 * Returns:
 *			void *   - NULL
 * Side-Effects:
 *
 * Description: Thread responsible for handling client messages. It will check
 * 				for the client message correctness and ignore all messages
 * 				that contain wrong values or don't follow the protocol.
 ******************************************************************************/
void * client_thread (void* arg)
{
	int  	n, fd, ff_fd, ft_fd;
	char 	buffer[BUFF_SIZE];

	fd = *((int*) arg);

	memset(buffer, ' ', BUFF_SIZE*sizeof(char));

	/* initializes fifo files with -1*/
	ff_fd = -1;
	ft_fd = -1;


	if (client_setup(fd,&ff_fd, &ft_fd) == -1) 				{client_disconnect(fd,ff_fd,ft_fd); return NULL;}

	if (client_loop(fd,ff_fd, ft_fd) == -1)					{client_disconnect(fd,ff_fd,ft_fd); return NULL;}


	/* sends dc message*/
	n = sprintf(buffer, "DC\n");
	buffer[n] = '\0';
	send(fd,buffer, BUFF_SIZE,0);

	debug_print("CLIENT" ,buffer, pthread_self(),1,debug);


	/* closes opened files*/
	close(fd); 
	close(ff_fd);
	close(ft_fd);


	/*removes client from list*/
	pthread_rwlock_wrlock(&lock_clients);
	pthread_mutex_lock(&lock_success);

	remove_client(&clients_head,(unsigned long)pthread_self());

	if (number_of_clients(clients_head) == 0)
	{	
		shut_down_success = 1;
	}
	pthread_mutex_unlock(&lock_success);
	pthread_rwlock_unlock(&lock_clients);
	

	return NULL;
}

                 /***************************************
                 *       AUXILIAR (THREAD) FUNCTIONS    *
                 * *************************************/



/******************************************************************************
 * client_setup() 
 *
 * Arguments:
 *			int      - socket ID file.
 * 			int * 	 - pointer to plays fifo file (empty). To be filed by this 
 					   function.
 * 			int *    - pointer to fruits fifo file (empty). To be filed by this 
 					   function.
 * Returns:
 *			void *   - NULL
 * Side-Effects:
 *
 * Description: Called by client thread. Establishes client connection and 
 * 				sends all needed information (board, pieces) to the client.
 * 				Generates client coordenates and adds client to list. Updates
 * 				number of max fruits and empty places.
 ******************************************************************************/
int client_setup(int fd, int* pff_fd, int* pft_fd)
{	

	int 	n,i,j,num_pieces, first_client;

	int 	pac_rgb[3];
	int 	mon_rgb[3];

	int 	frt_rgb[3];
	

	char 	buffer[BUFF_SIZE];
	char 	buffer_aux[PIECE_SIZE];

	memset(buffer,' ',BUFF_SIZE*sizeof(char));
	memset(frt_rgb, 0 , 3*sizeof(int));


	/* tries to opens fifo */
	if ((*pff_fd = open(FIFO_FILE, O_WRONLY))== -1) 		
	{
		/*if it fails to open, tries to create */
		if (mkfifo(FIFO_FILE, 0666)!=0)								{func_err("mkfifo"); return -1; }

		/* tries to open again */
		if ((*pff_fd = open(FIFO_FILE, O_WRONLY))== -1)				{func_err("open");   return -1; }
	}

	debug_print("CLIENT" ,"Opened Plays FIFO for Write", pthread_self(),2,debug);

	/* tries to opens fifo */
	if ((*pft_fd = open(FRUIT_FIFO_FILE, O_WRONLY))== -1) 		
	{	
		/*if it fails to open, tries to create */
		if (mkfifo(FRUIT_FIFO_FILE, 0666)!=0)						{func_err("mkfifo"); return -1; }
		
		/* tries to open again */
		if ((*pft_fd = open(FRUIT_FIFO_FILE, O_WRONLY))== -1)		{func_err("open");   return -1; }
	}

	debug_print("CLIENT" ,"Opened Fruits FIFO for Write", pthread_self(),2,debug);

	pthread_mutex_lock(&lock_empty);

	if (status.empty >= 4)
	{
		n = sprintf(buffer,"WE # %lx\n", pthread_self());

		buffer[n] = '\0';

		if (send(fd,buffer,BUFF_SIZE,0) == -1) 						{func_err("send"); pthread_mutex_unlock(&lock_empty); return -1;}

		debug_print("CLIENT" ,buffer, pthread_self(),1,debug);

		pthread_rwlock_rdlock(&lock_clients);

		if(number_of_clients(clients_head) == 0)
		{
			/* decrements by 2 for first user -  for pacman and monters */
			status.empty -= 2;
			pthread_mutex_unlock(&lock_empty);
			pthread_rwlock_unlock(&lock_clients);
			first_client = 1;
		}
		else
		{
			/* decrements by 4 for each user - 2 for pacman and monters, 2 for the extra fruits */
			status.empty -= 4;
			pthread_mutex_unlock(&lock_empty);
			pthread_rwlock_unlock(&lock_clients);

			/* updates max number of fruits*/
			pthread_mutex_lock(&lock_fruits);
			status.max_fruits +=2;
			pthread_mutex_unlock(&lock_fruits);

			first_client = 0;
		}
		

	}

	else	/* all seats taken */
	{
		pthread_mutex_unlock(&lock_empty);

		/* notifies user */
		n = sprintf(buffer,"W8\n");
		buffer[n] = '\0';

		debug_print("CLIENT" ,buffer, pthread_self(),1,debug);

		if (send(fd,buffer,BUFF_SIZE,0) == -1 ) 		{func_err("send"); return -1;}

		return -1;
	}


	if ( (n = recv(fd, buffer, BUFF_SIZE, 0) )== -1) 	{func_err("recv"); return -1;}
	buffer[n] = '\0';

	debug_print("CLIENT" ,buffer, pthread_self(),0,debug);

	if (strstr(buffer, "CC") == NULL) 					{inv_msg(buffer); return -1;}

	

	if ( sscanf(buffer, "%*s [%d,%d,%d] [%d,%d,%d]\n", &pac_rgb[0], &pac_rgb[1], &pac_rgb[2], &mon_rgb[0], &mon_rgb[1], &mon_rgb[2] ) != 6) 	{inv_format(buffer); return -1;}

	/* checks if client's rgb is correct */
	if ((rgb_verf(pac_rgb[0], pac_rgb[1], pac_rgb[2]) == -1) || (rgb_verf(mon_rgb[0], mon_rgb[1], mon_rgb[2]) == -1)) return -1;
	/*places pacman and monster in random positions*/

	place_random_position(status.board, status.row, status.col, PACMAN,  pac_rgb, *pff_fd);
	place_random_position(status.board, status.row, status.col, MONSTER,  mon_rgb, *pff_fd);


	/* places the fruits for a not first client*/
	if(!first_client) 				write_fruit(*pft_fd, *pff_fd, 1);


	/* non empty pieces */

	num_pieces = status.row*status.col - status.empty;

	n = sprintf(buffer, "MP %d:%d\n", status.row, status.col);
	buffer[n] = '\0';

	debug_print("CLIENT" ,buffer, pthread_self(),1,debug);

	if (send(fd,buffer,BUFF_SIZE,0) == -1)				{func_err("send"); return -1;}


	num_pieces = 0;

	/* locks clients for writting new client */
	pthread_rwlock_wrlock(&lock_clients);

	add_client(&clients_head, (unsigned long) pthread_self(), fd, pac_rgb, mon_rgb);

	/*unlocks clients*/
	pthread_rwlock_unlock(&lock_clients);

	/*sends all piece information to the client*/
	
	for (i = 0; i < status.row; i++)
	{
		for (j = 0 ; j < status.col; j++)
		{
			if(is_empty(i,j,status.board)) 				continue;

			n = sprintf(buffer, "PT %s\n", print_piece(status.board,i,j,buffer_aux));
			buffer[n] = '\0';

			if (send(fd,buffer,BUFF_SIZE, 0) == -1) 	{func_err("send"); return -1;}

			debug_print("CLIENT" ,buffer, pthread_self(),1,debug);
			num_pieces++;
		}
	}


	/* sends summary of sent data */
	n = sprintf(buffer, "SS %d\n", num_pieces);
	buffer[n] = '\0';

	if (send(fd,buffer,BUFF_SIZE, 0) == -1) 			{func_err("send"); return -1;}

	debug_print("CLIENT" ,buffer, pthread_self(),1,debug);


	/* waits for client to answer*/
	if ( (n = recv(fd, buffer,BUFF_SIZE,0) ) == -1) 	{func_err("recv"); return -1;}


	/* cleans buffer */
	buffer[n] = '\0';

	debug_print("CLIENT" ,buffer, pthread_self(),0,debug);

	/* connection request*/	
	if(strstr(buffer, "OK") == NULL) 					{inv_msg(buffer); return -1;}


	/* setup completed */
	debug_print("CLIENT" ,"Client Setup Completed...", pthread_self(),2,debug);

	return 0;
}

/******************************************************************************
 * client_loop() 
 *
 * Arguments:
 *			int      - socket ID file.
 * 			int 	 - plays fifo file.
 * 			int      - fruits fifo file
 * Returns:
 *			int      - Returns -1 on error or broken connection.
 * Side-Effects:
 *
 * Description: Called by client thread. Implements the client loop. It reads 
 * 				all incoming messages from the client, checks for its correctness
 * 				and writes the plays to the board and main.
 ******************************************************************************/

int client_loop(int fd, int ff_fd, int ft_fd)
{

	int 				n, piece, prev_x, prev_y, x, y, new_y, new_x;
	int 				pac_rgb[3], mon_rgb[3], coord[4];
	char 				buffer[BUFF_SIZE];
	struct 	timeval 	tv, start, end;

	long				diff;



	/* sets time struct to 1 seconds  */
    tv.tv_sec  = 1;
    tv.tv_usec = 0;


	memset(buffer,' ',BUFF_SIZE*sizeof(char));

	
	/* sets a timeout of 0 second => removes timeout*/
	setsockopt(fd,SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);


	get_pac_rgb(clients_head,(unsigned long)pthread_self(),&pac_rgb[0],&pac_rgb[1] ,&pac_rgb[2]);

	get_mon_rgb(clients_head,(unsigned long)pthread_self(),&mon_rgb[0],&mon_rgb[1], &mon_rgb[2]);

	/* gets current time*/
	gettimeofday(&start,NULL);


	while(!shut_down)
	{
		n = recv(fd, buffer, BUFF_SIZE, 0);

		/* 0 bytes received*/
		if (n == 0) continue;

		/* timedout */
		if  (n == -1)
		{	
			/* sends keep alive message*/
			n = sprintf(buffer, "KEEP ALIVE\n");
			buffer[n] = '\0';

			/* if -1, client disconnected*/
			if (send(fd,buffer, BUFF_SIZE,0)  == -1) 				return -1;

			/*debug_print("CLIENT" ,buffer, pthread_self(),1,debug);*/

			/* gets current time*/
			gettimeofday(&end,NULL);

			/* calculates how long the user has been inative*/
			diff = (end.tv_usec - start.tv_usec) + (end.tv_sec - start.tv_sec)*1000000;


			/*user inative for too long*/
			if (diff >= USER_TIMEOUT_USECONDS)
			{

				for (n = 0; n < status.col ; n++) 					pthread_mutex_lock(&lock_col[n]);
				/* cleans pacman and monster from board */
				clear_player(status.board, status.row, status.col, (unsigned long) pthread_self(), coord);

				for (n = 0; n < status.col ; n++) 					pthread_mutex_unlock(&lock_col[n]);

				/* generates new positions and clears hold ones*/
				if(coord[0] != -1 && coord[1] != -1 && coord[2] != -1 && coord[3] != -1)
				{
					
					n = sprintf(buffer, "CL %d:%d\n", coord[2], coord[3]);
					buffer[n] = '\0';
					write_play_to_main(buffer,ff_fd);

					n = sprintf(buffer, "CL %d:%d\n", coord[0], coord[1]);
					buffer[n] = '\0';
					write_play_to_main(buffer,ff_fd);

					place_random_position(status.board, status.row, status.col, PACMAN,  pac_rgb, ff_fd);
					place_random_position(status.board, status.row, status.col, MONSTER,  mon_rgb, ff_fd);
				}
				/*updates clock*/
				gettimeofday(&start,NULL);
			}

			continue;
		}
		

		buffer[n] = '\0';

		debug_print("CLIENT" ,buffer, pthread_self(),0,debug);


		if(strstr(buffer, "MV") != NULL)
		{
			gettimeofday(&end,NULL);

			diff = (double) (end.tv_usec - start.tv_usec) + (double) (end.tv_sec - start.tv_sec)*1000000;

			if(diff < USER_MAX_TIME_USECONDS)																		continue;

			if ( sscanf(buffer, "%*s %d @ %d:%d => %d:%d", &piece, &prev_x, &prev_y, &x,&y) != 5) 					{inv_format(buffer); continue;}

			/* out of boundaries*/
			if ( (x > status.row) || (y > status.col) || x < -1 || y < -1 
			|| prev_x >= status.row || prev_y >= status.col || prev_x < 0 || prev_y < 0 )							continue;

			/* big move*/
			if ((abs(prev_x-x) + abs(prev_y-y)) != 1 )																continue;			

			/*check for map bounce*/
			if ( (x == status.row ||  y == status.col || x == -1 || y == -1))
			{
				if (!bounce(status.board,status.row,status.col,x, y, prev_x, prev_y, &new_x, &new_y)) 				continue;

				x = new_x;
			 	y = new_y;
			}

			/* pacman movement */
			if (piece == PACMAN) 	
			{
				if (!pacman_movement(status.board, status.row, status.col,x,y,prev_x,prev_y,ff_fd,ft_fd, pac_rgb))  continue;

				/*updates clock*/
				gettimeofday(&start,NULL);

			}

			/* monster movement */
			else if ( piece == MONSTER)	
			{

				if (!monster_movement(status.board, status.row, status.col,x,y,prev_x,prev_y,ff_fd,ft_fd, mon_rgb)) continue;

				/*updates clock*/
				gettimeofday(&start,NULL);

			}

			else if (piece == POWER_PACMAN)
			{

				if (!power_pacman_movement(status.board, status.row, status.col,x,y,prev_x,prev_y,ff_fd,ft_fd, mon_rgb)) continue;

				/*updates clock*/
				gettimeofday(&start,NULL);
			}

			else 		 										inv_piece(piece);

		}

		else if(strstr(buffer, "DC"))							return -1;

		else													{inv_msg(buffer);}
	}


	return 0;
}

/******************************************************************************
 * client_disconnect() 
 *
 * Arguments:
 *			int      - socket ID file.
 * 			int 	 - plays fifo file.
 * 			int      - fruits fifo file
 * Returns:
 *			int      - Returns -1 on error or broken connection.
 * Side-Effects:
 *
 * Description: Called by client thread. Implements the client disconnect
 * 				Only called on a error in setup or a disconnect from the client
 * 				side. It cleans up the client from the board, client list and
 * 				user interface. Updates number of max fruits and empty places.
 ******************************************************************************/
void client_disconnect(int fd, int ff_fd, int ft_fd)
{
	int 	n;
	char 	buffer[BUFF_SIZE];
	int 	coord[4];

	memset(buffer,' ',BUFF_SIZE*sizeof(char));
	memset(coord, -1, 4*sizeof(int));

	

	/* closes socket */
	if (fd != -1 ) 			close(fd);	

	pthread_rwlock_wrlock(&lock_clients);

	/* removes client if finds a match */
	remove_client(&clients_head,(unsigned long)pthread_self());

	pthread_rwlock_unlock(&lock_clients);

	
	for (n = 0; n < status.col ; n++) 			pthread_mutex_lock(&lock_col[n]);

	/* cleans pacman and monster from board */
	clear_player(status.board, status.row, status.col, (unsigned long) pthread_self(), coord);
	
	for (n = 0; n < status.col ; n++) 			pthread_mutex_unlock(&lock_col[n]);

	if(coord[0] != -1 && coord[1] != -1)
	{
		
		n = sprintf(buffer, "CL %d:%d\n", coord[0], coord[1]);
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);
	} 	
	if(coord[2] != -1 && coord[3] != -1)
	{
		n = sprintf(buffer, "CL %d:%d\n", coord[2], coord[3]);
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);
	}	


	pthread_mutex_lock(&lock_empty);
	status.empty+=4;
	pthread_mutex_unlock(&lock_empty);


	pthread_mutex_lock(&lock_fruits);
	if (status.max_fruits != 0)
		status.max_fruits-=2;
	pthread_mutex_unlock(&lock_fruits);


	/* closes fifos */
	if(ff_fd != -1 ) 	close(ff_fd);

	if(ft_fd != -1) 	close(ft_fd);
}

/******************************************************************************
 * write_play_to_main() 
 *
 * Arguments:
 *			char *   - message to be printed
 * 			int 	 - plays fifo file.
 * Returns:
 *			int      - Returns -1 on error, 0 on success.
 * Side-Effects:
 *
 * Description: Called by client thread and fruit thread. Writes to the main
 * 				thread the plays to send to the users and display on the 
 * 				interface.
 ******************************************************************************/
int write_play_to_main(char* play, int fd)
{


	if (play == NULL)									return -1;
	
	debug_print("CLIENT" ,play, pthread_self(),1,debug);

	/* writes to fifo*/
	if (write(fd, play, BUFF_SIZE) == -1) 				{func_err("write"); return -1;}
	
	return 0;
}

/******************************************************************************
 * write_fruit() 
 *
 * Arguments:
 *			int      - fruits fifo file.
 * 			int 	 - plays fifo file.
 * 			int   	 - flag (0 - if its a spawn request on the normal time)
 							(1 - if its a imediate spawn request)
 * Returns:
 *			int      - Returns -1 on error, 0 on success.
 * Side-Effects:
 *
 * Description: Called by client thread when it needs a fruit to be placed on 
 * 				board. If it is during the setup, it will ask for a imediate 
 * 				spawn and the fruit will be printed right away, if not, it 
 * 				will build a message with the current time and sent it to the
 * 				fruit thread to process.
 ******************************************************************************/
int write_fruit(int ft_fd, int fd, int now)
{
	
	int 				i, n, lemon;
	int 				rgb[3];
	char 				buffer[BUFF_SIZE];
	struct timeval 		tv;

	memset(buffer, ' ', BUFF_SIZE*sizeof(char));
	memset(rgb, 0, 3*sizeof(int));

	if (now)
	{
		srand(time(NULL));

		for(i = 0; i < FRUITS_PER_PLAYER; i++)
		{
			/* gets random fruit*/
			lemon = rand()%2;

			if(lemon)
			{
				place_random_position(status.board, status.row, status.col,LEMON, rgb, fd);
			}
			else
			{
				place_random_position(status.board, status.row, status.col,CHERRY, rgb, fd);
			}
		}
	
		pthread_mutex_lock(&lock_cur);
		status.cur_fruits+=FRUITS_PER_PLAYER;
		pthread_mutex_unlock(&lock_cur);
	}

	else
	{	
		gettimeofday(&tv,NULL);
		/* sends curr time to fruit thread */
		n = sprintf(buffer,"TM %ld %ld\n", tv.tv_sec , tv.tv_usec);
		buffer[n] = '\0';

		/* writes to fifo*/
		if (write(ft_fd, buffer, BUFF_SIZE) == -1) 				{func_err("write"); return -1;}

		debug_print("CLIENT" ,buffer, pthread_self(),1,debug);
	}

	
	
	return 0;
}

/******************************************************************************
 * server_disconnect() 
 *
 * Arguments:
 *			void
 * 		
 * Returns:
 *			void
 * Side-Effects:
 *
 * Description: Cleans up the server. Closes the board interface window. Free's
 * 				the board memory, waits for all clients threads to finish 
 * 				execution and notify the client of the disconnect. Free's client
 * 				list and destroys all mutexes.
 ******************************************************************************/
void server_disconnect()
{

	int i;

	close_board_windows();

	pthread_rwlock_rdlock(&lock_clients);
	
	/* if number of clients is zero all threads have finished*/
	if (number_of_clients(clients_head) != 0)
	{	

		pthread_rwlock_unlock(&lock_clients);

		while (1)
		{
			pthread_mutex_lock(&lock_success);

			if (shut_down_success == 1)
			{
				pthread_mutex_unlock(&lock_success);
				break;
			}
			pthread_mutex_unlock(&lock_success);

			/* sleeps a bit after checking again */
			usleep(500);
		}
	}
	else
	{
		pthread_rwlock_unlock(&lock_clients);
	}
	
	

	/* frees head */
	free(clients_head);

	/* frees board */
	free_board(status.row, status.board);
	
	/* destroys mutexes */
	for(i = 0; i < status.col ; i++)
		if (pthread_mutex_destroy(&lock_col[i])) 			func_err("pthread_mutex_destroy");
	
	/* frees mutexes */
	free(lock_col);
	
	if (pthread_rwlock_destroy(&lock_clients)) 				func_err("pthread_mutex_destroy");
	if (pthread_mutex_destroy(&lock_empty)) 				func_err("pthread_mutex_destroy");
	if (pthread_mutex_destroy(&lock_fruits)) 				func_err("pthread_mutex_destroy");
	if (pthread_mutex_destroy(&lock_cur)) 					func_err("pthread_mutex_destroy");
	if (pthread_mutex_destroy(&lock_success))				func_err("pthread_mutex_destroy");

}

                 /***************************************
                 *         GAME RELATED FUNCTIONS       *
                 * *************************************/


/******************************************************************************
 * pacman_movement() 
 *
 * Arguments:
 *			board_piece** 	- pointer to the board struct.
 * 			int 			- number of rows of the board.
 * 			int 			- number of cols of the board.
 * 			int 			- destination x coordenate.
 * 			int 			- destination y coordenate.
 * 			int 			- previous x coordenate.
 * 			int 			- previous y coordenate.
 * 			int 			- plays fifo file.
 * 			int 			- fruits fifo file.
 * 			int * 			- rgb colours of the piece.
 * Returns:
 *			int      - Returns 0 if the piece didn't move, 1 if it moved
 * Side-Effects:
 *
 * Description: Handles pacman movement, responsable for checking type of
 * 				movement and making the necessary changes accordingly. 
 * 				Sends the main thread the information about the movement
 * 				made.
 ******************************************************************************/
int pacman_movement(board_piece** board,int row, int col, int x, int y, int prev_x , int prev_y, int ff_fd, int ft_fd, int* rgb)
{
	int 			n, new_x, new_y, fruit, player;

	char 			buffer[BUFF_SIZE];
	char 			buffer_aux[PIECE_SIZE];

	unsigned long 	id;

	memset(buffer,' ',BUFF_SIZE*sizeof(char));

	fruit=player=0;

	id = pthread_self();
		
	/* next position is a brick */
	if (is_brick(x,y,board))
	{
		if (!bounce(board, row, col,x, y, prev_x, prev_y, &new_x, &new_y)) 	return 0;

		/* updates values of x and y conserning the bounce*/
		x = new_x;
		y = new_y;
	}

	/* every function above only checks if a position on board is a brick 
	 * or out of bounds. In the game enviroment bricks and bounds are 
	 * STATIC and can't be overwritten so there is no race condition to check 
	 * if a place is a brick or out of bounds. Knowing this it is possible
	 * to obtain the next bounce position without having to lock board memory.
	 * This allows us to only have to always lock one col per move, even if 
	 * the movement is horizontal because we can safely compute the bounce 
	 * result that is independent from possible memory updates. Given the
	 * character trades also have to lock the prev position to garantee
	 * a safe swap. In regards to the eating mechanic a trylock aproach is used
	 * when finding the new random position, to avoid possible deadlocks*/

	if(prev_x==x && prev_y == y) 		return 0;

	/* horizontal */
	if (prev_x == x)
	{	

		/* locks lower indexed mutex first*/
		if (y < prev_y)
		{
			/* target position */
			pthread_mutex_lock(&lock_col[y]);

			/* possible swap position */
			pthread_mutex_lock(&lock_col[prev_y]);
		}

		else
		{
			/* possible swap position */
			pthread_mutex_lock(&lock_col[prev_y]);

			/* target position */
			pthread_mutex_lock(&lock_col[y]);

			
		}
	}

	/* vertical */
	else if (prev_y==y)
	{
		/* target position and possible swap position */
		pthread_mutex_lock(&lock_col[y]);
	}


	/* check if client send correct last position*/
	if (!piece_is_correct(prev_x,prev_y,PACMAN,pthread_self(),status.board))
	{
		/* horizontal */
		if (prev_x == x)
		{	
			/* target position */
			pthread_mutex_unlock(&lock_col[y]);

			/* possible swap position */
			pthread_mutex_unlock(&lock_col[prev_y]);
		}

		/* vertical */
		else if (prev_y==y)
		{
			/* target position and possible swap position */
			pthread_mutex_unlock(&lock_col[y]);
		}

		return 0;
	}

	/* next position is empty */
	if (is_empty(x, y, board))
	{
		/* moves pacman */	
		move_and_clear(board,prev_x, prev_y, x, y);

		n = sprintf(buffer, "PT %s\n", print_piece(board, x, y,buffer_aux));
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);

		n = sprintf(buffer, "CL %d:%d\n", prev_x, prev_y);
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);

	}

	/* hits enemy pacman */
	else if (is_pacman(x,y,board) || is_power_pacman(x,y,board))
	{
		/* characters change positions */
		switch_pieces(board,prev_x, x, prev_y, y);

		n = sprintf(buffer, "PT %s\n", print_piece(board, x, y,buffer_aux));
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);

		n = sprintf(buffer, "PT %s\n", print_piece(board, prev_x, prev_y,buffer_aux));
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);

	}

	/* hits monster*/
	else if( is_monster(x,y,board))
	{

		/* friendly monster*/
		if( (id = get_id(board, x, y) ) == pthread_self())
		{
			/* pacman and monster change positions*/
			switch_pieces(board,prev_x, x, prev_y, y);

			n = sprintf(buffer, "PT %s\n", print_piece(board, x, y,buffer_aux));
			buffer[n] = '\0';
			write_play_to_main(buffer,ff_fd);

			n = sprintf(buffer, "PT %s\n", print_piece(board, prev_x, prev_y,buffer_aux));
			buffer[n] = '\0';
			write_play_to_main(buffer,ff_fd);

		}

		/* enemy monster*/
		else
		{
			player_eats_player(board, row, col, x, y, prev_x, prev_y,1, ff_fd);
			player = 1;
		}
	}

	else if (is_any_fruit(x,y,board))
	{

		move_and_clear(board,prev_x, prev_y, x, y);

		/*transform pacman in super pacman*/
		transform_pacman(board,x,y,POWER_PACMAN_COUNTER);


		n = sprintf(buffer, "PT %s\n", print_piece(board, x, y,buffer_aux));
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);

		n = sprintf(buffer, "CL %d:%d\n", prev_x, prev_y);
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);

		fruit = 1;

		/* updates curr fruits number*/
		pthread_mutex_lock(&lock_cur);
		status.cur_fruits--;
		pthread_mutex_unlock(&lock_cur);


		/* writes fruit*/
		write_fruit(ft_fd, ff_fd, 0);
	}
	else	/* invalid piece */
	{

		/* horizontal */
		if (prev_x == x)
		{	
			pthread_mutex_unlock(&lock_col[y]);
			pthread_mutex_unlock(&lock_col[prev_y]);
		}

		/* vertical */
		else if (prev_y==y)
		{
			pthread_mutex_unlock(&lock_col[y]);
		}
	
		return -1;
	}

	/* horizontal */
	if (prev_x == x)
	{	
		pthread_mutex_unlock(&lock_col[y]);
		pthread_mutex_unlock(&lock_col[prev_y]);
	}

	/* vertical */
	else if (prev_y==y)
	{
		pthread_mutex_unlock(&lock_col[y]);
	}

	/* NON CRITICAL BOARD ZONE */
	if(fruit)
	{
		/* updates score */
		pthread_rwlock_wrlock(&lock_clients);
		inc_fruit_score(clients_head, pthread_self());
		pthread_rwlock_unlock(&lock_clients);
	}
	else if (player)
	{
		/* updates score */
		pthread_rwlock_wrlock(&lock_clients);
		inc_player_score(clients_head, id);
		pthread_rwlock_unlock(&lock_clients);	
	}


	return 1;
}

/******************************************************************************
 * power_pacman_movement() 
 *
 * Arguments:
 *			board_piece** 	- pointer to the board struct.
 * 			int 			- number of rows of the board.
 * 			int 			- number of cols of the board.
 * 			int 			- destination x coordenate.
 * 			int 			- destination y coordenate.
 * 			int 			- previous x coordenate.
 * 			int 			- previous y coordenate.
 * 			int 			- plays fifo file.
 * 			int 			- fruits fifo file.
 * 			int * 			- rgb colours of the piece.
 * Returns:
 *			int      - Returns 0 if the piece didn't move, 1 if it moved
 * Side-Effects:
 *
 * Description: Handles power pacman movement, responsable for checking type of
 * 				movement and making the necessary changes accordingly. 
 * 				Sends the main thread the information about the movement
 * 				made.
 ******************************************************************************/
int power_pacman_movement(board_piece** board,int row, int col, int x, int y, int prev_x , int prev_y, int ff_fd, int ft_fd, int* rgb)
{
	int 	n, new_x, new_y, fruit, player;

	char 	buffer[BUFF_SIZE];
	char 	buffer_aux[PIECE_SIZE];

	memset(buffer,' ',BUFF_SIZE*sizeof(char));
	
	/* initializes flags*/
	fruit = player = 0;

	/* next position is a brick */
	if (is_brick(x,y,board))
	{
		if (!bounce(board, row, col,x, y, prev_x, prev_y, &new_x, &new_y)) 	return 0;

		/* updates values of x and y conserning the bounce*/
		x = new_x;
		y = new_y;
	}

	/* every function above only checks if a position on board is a brick 
	 * or out of bounds. In the game enviroment bricks and bounds are 
	 * STATIC and can't be overwritten so there is no race condition to check 
	 * if a place is a brick or out of bounds. Knowing this it is possible
	 * to obtain the next bounce position without having to lock board memory.
	 * This allows us to only have to always lock one col per move, even if 
	 * the movement is horizontal because we can safely compute the bounce 
	 * result that is independent from possible memory updates. Given the
	 * character trades also have to lock the prev position to garantee
	 * a safe swap. In regards to the eating mechanic a trylock aproach is used
	 * when finding the new random position, to avoid possible deadlocks*/

	if(prev_x==x && prev_y == y) 		return 0;

	/* horizontal */
	if (prev_x == x)
	{	

		/* locks lower indexed mutex first*/
		if (y < prev_y)
		{
			/* target position */
			pthread_mutex_lock(&lock_col[y]);

			/* possible swap position */
			pthread_mutex_lock(&lock_col[prev_y]);
		}

		else
		{
			/* possible swap position */
			pthread_mutex_lock(&lock_col[prev_y]);

			/* target position */
			pthread_mutex_lock(&lock_col[y]);

			
		}
	}

	/* vertical */
	else if (prev_y==y)
	{
		/* target position and possible swap position */
		pthread_mutex_lock(&lock_col[y]);
	}

	/* check if client send correct last position*/
	if (!piece_is_correct(prev_x,prev_y,POWER_PACMAN,pthread_self(),status.board))
	{
		/* horizontal */
		if (prev_x == x)
		{	
			/* target position */
			pthread_mutex_unlock(&lock_col[y]);

			/* possible swap position */
			pthread_mutex_unlock(&lock_col[prev_y]);
		}

		/* vertical */
		else if (prev_y==y)
		{
			/* target position and possible swap position */
			pthread_mutex_unlock(&lock_col[y]);
		}

		return 0;
	}

	/* next position is empty */
	if (is_empty(x, y, board))
	{
		/* moves pacman */	
		move_and_clear(board,prev_x, prev_y, x, y);

		n = sprintf(buffer, "PT %s\n", print_piece(board, x, y,buffer_aux));
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);

		n = sprintf(buffer, "CL %d:%d\n", prev_x, prev_y);
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);

	}

	/* hits enemy pacman */
	else if (is_pacman(x,y,board) || is_power_pacman(x,y,board))
	{
		/* characters change positions */
		switch_pieces(board,prev_x, x, prev_y, y);

		n = sprintf(buffer, "PT %s\n", print_piece(board, x, y,buffer_aux));
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);

		n = sprintf(buffer, "PT %s\n", print_piece(board, prev_x, prev_y,buffer_aux));
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);

	}

	/* hits monster*/
	else if(is_monster(x,y,board))
	{

		/* friendly monster*/
		if(get_id(board, x, y) == pthread_self())
		{
			/* pacman and monster change positions*/
			switch_pieces(board,prev_x, x, prev_y, y);

			n = sprintf(buffer, "PT %s\n", print_piece(board, x, y,buffer_aux));
			buffer[n] = '\0';
			write_play_to_main(buffer,ff_fd);

			n = sprintf(buffer, "PT %s\n", print_piece(board, prev_x, prev_y,buffer_aux));
			buffer[n] = '\0';
			write_play_to_main(buffer,ff_fd);

		}

		/* enemy monster*/
		else
		{
			/* if max monster eaten - reverses pacman */
			if (decrement_counter(board,prev_x,prev_y) == 0)
			{
				reverse_pacman(board,prev_x,prev_y);
			}

			player_eats_player(board, row, col, x, y, prev_x, prev_y, 0, ff_fd);

			/* sets flag a player was eaten*/
			player = 1;
		}
	}

	else if (is_any_fruit(x,y,board))
	{

		move_and_clear(board,prev_x, prev_y, x, y);

		n = sprintf(buffer, "PT %s\n", print_piece(board, x, y,buffer_aux));
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);

		n = sprintf(buffer, "CL %d:%d\n", prev_x, prev_y);
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);

		/* updates curr fruits number*/
		pthread_mutex_lock(&lock_cur);
		status.cur_fruits--;
		pthread_mutex_unlock(&lock_cur);


		/* writes fruit*/
		write_fruit(ft_fd, ff_fd, 0);


		/* sets flag a fruit was eaten */
		fruit = 1;
	}
	else	/* invalid piece */
	{

		/* horizontal */
		if (prev_x == x)
		{	
			pthread_mutex_unlock(&lock_col[y]);
			pthread_mutex_unlock(&lock_col[prev_y]);
		}

		/* vertical */
		else if (prev_y==y)
		{
			pthread_mutex_unlock(&lock_col[y]);
		}
	
		return -1;
	}

	/* horizontal */
	if (prev_x == x)
	{	
		pthread_mutex_unlock(&lock_col[y]);
		pthread_mutex_unlock(&lock_col[prev_y]);
	}

	/* vertical */
	else if (prev_y==y)
	{
		pthread_mutex_unlock(&lock_col[y]);
	}


	/* NON CRITICAL BOARD ZONE */
	if(fruit)
	{
		/* updates score */
		pthread_rwlock_wrlock(&lock_clients);
		inc_fruit_score(clients_head, pthread_self());
		pthread_rwlock_unlock(&lock_clients);
	}
	else if (player)
	{
		/* updates score */
		pthread_rwlock_wrlock(&lock_clients);
		inc_player_score(clients_head, pthread_self());
		pthread_rwlock_unlock(&lock_clients);	
	}


	return 1;
}

/******************************************************************************
 * monster_movement() 
 *
 * Arguments:
 *			board_piece** 	- pointer to the board struct.
 * 			int 			- number of rows of the board.
 * 			int 			- number of cols of the board.
 * 			int 			- destination x coordenate.
 * 			int 			- destination y coordenate.
 * 			int 			- previous x coordenate.
 * 			int 			- previous y coordenate.
 * 			int 			- plays fifo file.
 * 			int 			- fruits fifo file.
 * 			int * 			- rgb colours of the piece.
 * Returns:
 *			int      - Returns 0 if the piece didn't move, 1 if it moved
 * Side-Effects:
 *
 * Description: Handles monster movement, responsable for checking type of
 * 				movement and making the necessary changes accordingly. 
 * 				Sends the main thread the information about the movement
 * 				made.
 ******************************************************************************/
int monster_movement(board_piece** board, int row, int col, int x, int y, int prev_x , int prev_y, int ff_fd, int ft_fd, int* rgb)
{
	int 			n, new_x, new_y, fruit, player;

	char 			buffer[BUFF_SIZE];
	char 			buffer_aux[PIECE_SIZE];

	unsigned long 	id;

	fruit = player = 0;

	memset(buffer,' ',BUFF_SIZE*sizeof(char));

	id = pthread_self();

	/* next position is a brick */
	if (is_brick(x,y,board))
	{
		if (!bounce(board, row, col,x, y, prev_x, prev_y, &new_x, &new_y)) 	return 0;


		/* updates values of x and y conserning the bounce*/
		x = new_x;
		y = new_y;
	}

	/* every function above only checks if a position on board is a brick 
	 * or out of bounds. In the game enviroment bricks and bounds are 
	 * STATIC and can't be overwritten so there is no race condition to check 
	 * if a place is a brick or out of bounds. Knowing this it is possible
	 * to obtain the next bounce position without having to lock board memory.
	 * This allows us to only have to always lock one col per move, even if 
	 * the movement is horizontal because we can safely compute the bounce 
	 * result that is independent from possible memory updates. Given the
	 * character clear and trades also have to lock the prev position to garantee
	 * a safe swap. In regards to the eating mechanic a trylock aproach is used
	 * when finding the new random position, to avoid possible deadlocks*/


	if(prev_x==x && prev_y==y)		return 0;

	/* horizontal */
	if (prev_x == x)
	{	

		/* locks lower indexed mutex first*/
		if (y < prev_y)
		{
			/* target position */
			pthread_mutex_lock(&lock_col[y]);

			/* possible swap position or clear*/
			pthread_mutex_lock(&lock_col[prev_y]);
		}

		else
		{
			/* possible swap position */
			pthread_mutex_lock(&lock_col[prev_y]);

			/* target position */
			pthread_mutex_lock(&lock_col[y]);

			
		}
	}

	/* vertical */
	else if (prev_y==y)
	{
		/* target position and possible swap position */
		pthread_mutex_lock(&lock_col[y]);
	}


	/* check if client send correct last position*/
	if (!piece_is_correct(prev_x,prev_y,MONSTER,pthread_self(),status.board))
	{
		/* horizontal */
		if (prev_x == x)
		{	
			/* target position */
			pthread_mutex_unlock(&lock_col[y]);

			/* possible swap position or clear*/
			pthread_mutex_unlock(&lock_col[prev_y]);
		}

		/* vertical */
		else if (prev_y==y)
		{
			/* target position and possible swap position */
			pthread_mutex_unlock(&lock_col[y]);
		}

		return 0;
	}

	/* next position is empty */
	if (is_empty(x, y, board))
	{
		/* moves monster */	
		move_and_clear(board,prev_x, prev_y, x, y);

		n = sprintf(buffer, "PT %s\n", print_piece(board, x, y,buffer_aux));
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);

		n = sprintf(buffer, "CL %d:%d\n", prev_x, prev_y);
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);

	}

	/* hits enemy monster */
	else if (is_monster(x,y,board))
	{
		/* characters change positions */
		switch_pieces(board,prev_x, x, prev_y, y);

		n = sprintf(buffer, "PT %s\n", print_piece(board, x, y,buffer_aux));
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);

		n = sprintf(buffer, "PT %s\n", print_piece(board, prev_x, prev_y,buffer_aux));
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);

	}

	/* hits pacman*/
	else if( is_pacman(x,y,board) || is_power_pacman(x,y,board))
	{

		/* friendly pacman*/
		if( (get_id(board, x, y)) == pthread_self())
		{
			/* pacman and monster change positions*/
			switch_pieces(board,prev_x, x, prev_y, y);	

			n = sprintf(buffer, "PT %s\n", print_piece(board, x, y,buffer_aux));
			buffer[n] = '\0';
			write_play_to_main(buffer,ff_fd);


			n = sprintf(buffer, "PT %s\n", print_piece(board, prev_x, prev_y,buffer_aux));
			buffer[n] = '\0';
			write_play_to_main(buffer,ff_fd);

		}

		/* enemy pacman*/
		else
		{
			if(is_pacman(x,y,board))		/* normal pacman */
				player_eats_player(board, row, col, x, y, prev_x, prev_y, 0, ff_fd);
			else							/* power pacman */
			{	if (decrement_counter(board,x,y) == 0)
				{
					reverse_pacman(board,x,y);
					
				}

				player_eats_player(board, row, col, x, y, prev_x, prev_y, 1, ff_fd);
				id = get_id(board,x,y);
			}

			/*sets player eaten flag*/
			player = 1;
		}
	}

	else if (is_any_fruit(x,y,board))
	{
		move_and_clear(board,prev_x, prev_y, x, y);

		n = sprintf(buffer, "PT %s\n", print_piece(board, x, y,buffer_aux));
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);

		n = sprintf(buffer, "CL %d:%d\n", prev_x, prev_y);
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);

		/* updates curr fruits number*/
		pthread_mutex_lock(&lock_cur);
		status.cur_fruits--;
		pthread_mutex_unlock(&lock_cur);

		/* writes fruit*/
		write_fruit(ft_fd, ff_fd, 0);

		fruit=1;
	}

	else	/* invalid piece */
	{
		

		/* horizontal */
		if (prev_x == x)
		{	
			pthread_mutex_unlock(&lock_col[y]);
			pthread_mutex_unlock(&lock_col[prev_y]);
		}

		/* vertical */
		else if (prev_y==y)
		{
			pthread_mutex_unlock(&lock_col[y]);
		}
	
		return -1;
	}

	/* horizontal */
	if (prev_x == x)
	{	
		pthread_mutex_unlock(&lock_col[y]);
		pthread_mutex_unlock(&lock_col[prev_y]);
	}

	/* vertical */
	else if (prev_y==y)
	{
		pthread_mutex_unlock(&lock_col[y]);
	}

	/* NON CRITICAL BOARD ZONE */
	if(fruit)
	{
		/* updates score */
		pthread_rwlock_wrlock(&lock_clients);
		inc_fruit_score(clients_head, id);
		pthread_rwlock_unlock(&lock_clients);
	}
	else if (player)
	{
		/* updates score */
		pthread_rwlock_wrlock(&lock_clients);
		inc_player_score(clients_head, id);
		pthread_rwlock_unlock(&lock_clients);	
	}


	return 1;
}

/******************************************************************************
 * bounce() 
 *
 * Arguments:
 *			board_piece** 	- pointer to the board struct.
 * 			int 			- number of rows of the board.
 * 			int 			- number of cols of the board.
 * 			int 			- destination x coordenate.
 * 			int 			- destination y coordenate.
 * 			int 			- previous x coordenate.
 * 			int 			- previous y coordenate.
 * 			int * 			- pointer to new x coordenate
 * 			int *			- pointer to new y coordenate
 * Returns:
 *			int      - Returns 0 if the piece didn't move, 1 if it moved
 * Side-Effects:
 *
 * Description: Calculates if the movement will result in a bounce or not
 * 				if it does updates the x and y coordenate to the input pointers.
 ******************************************************************************/
int bounce(board_piece** board, int row, int col,int x, int y, int prev_x, int prev_y, int *new_x, int*new_y)
{

	/*horizontal*/
	if (prev_x == x)
	{

		/* against left limit*/
		if 	(y==-1)
		{
			*new_y = 1;
			*new_x = x;
		} 

		/* against right limit*/		
		else if (y==col)
		{
			*new_y = col-2;
			*new_x = x;
		}	

		/* against	brick*/
		else
		{
			*new_y = prev_y + (prev_y-y);
			*new_x = x;
		}

			
	}

	/* vertical */
	else if(prev_y == y)
	{	

		/* against up limit*/
		if 	(x == -1)
		{
			*new_x = 1;
			*new_y = y;
		}

		/* against lower limit*/		 
		else if (x == row)
		{
			*new_x = row-2;
			*new_y = y;
		}

		/* agains brick */
		else
		{
			*new_x = prev_x + (prev_x - x);
			*new_y = y;
		}
	}

	else
	{
		return 1;
	}


	/* with the updated values calculates if there is another bounce */

	if (*new_x == -1 || *new_x == row || *new_y == col || *new_y == -1 || is_brick(*new_x,*new_y,board))
	{
		/* if next position is a bounce, stays where it is */
		return 0;
	}

	return 1;
}

/******************************************************************************
 * place_random_position() 
 *
 * Arguments:
 *			board_piece** 	- pointer to the board struct.
 * 			int 			- number of rows of the board.
 * 			int 			- number of cols of the board.
 * 			int 			- piece
 * 			int* 			- rgb colours of the piece.
 * 			int 			- plays fifo file.
 * Returns:
 *			void
 * Side-Effects:
 *
 * Description: Places the piece received in a new empty position. If there are
 * 				no empty positions, it leaves the piece where it is.
 ******************************************************************************/
void place_random_position(board_piece** board, int row, int col, int piece, int* rgb, int ff_fd)
{
	int 	new_row, new_col, n , last_col;
	char 	buffer[BUFF_SIZE];
	char  	buffer_aux[BUFF_SIZE];

	memset(buffer,' ',BUFF_SIZE*sizeof(char));
	memset(buffer_aux,' ',BUFF_SIZE*sizeof(char));

	/* gets a column at random*/
	srand(time(NULL));
	new_col = rand()%col;
	last_col = new_col;

	pthread_mutex_lock(&lock_empty);
	if (status.empty == 0) 									{pthread_mutex_unlock(&lock_empty); return;}
	pthread_mutex_unlock(&lock_empty);

	while(1)
	{
		/* tries to lock col*/
		if (!pthread_mutex_trylock(&lock_col[new_col]))
		{
			/* tries to get a random row for n-rows attempts */
			/* getting the right row - if there is any - can be seen as a bernoulli distribution with p = 1/n */
			/* therefore expected number of trials until sucess is ~ 1/p  = n */
			/* if it doesn't find a empty row within the n attempts it tries a new col*/
			for (n = 0; n < row ; n++)
			{
				
				new_row = rand()%row;

				if(is_empty(new_row, new_col,board))
				{

					place_piece(status.board,piece,new_row,new_col,(unsigned long) pthread_self(), rgb[0], rgb[1], rgb[2]);

					n = sprintf(buffer, "PT %s\n" , print_piece(board, new_row, new_col, buffer_aux));
					buffer[n] = '\0';
					write_play_to_main(buffer, ff_fd);

					/*unlocks lock*/
					pthread_mutex_unlock(&lock_col[new_col]);


					return;
				}
			}

			/* unlocks col*/
			pthread_mutex_unlock(&lock_col[new_col]);
		}

		/*changes col - doesn't repeat last col*/
		while(new_col==last_col)
		{
			last_col = new_col;
			new_col = rand()%col;
		}
	}
}

/******************************************************************************
 * player_eats_player() 
 *
 * Arguments:
 *			board_piece** 	- pointer to the board struct.
 * 			int 			- number of rows of the board.
 * 			int 			- number of cols of the board.
 * 			int 			- destination x coordenate.
 * 			int 			- destination y coordenate.
 * 			int 			- previous x coordenate.
 * 			int 			- previous y coordenate.
 * 			int 			- flag indicating if the player suicided (1)
 														   was eaten (0)
 * 			int 			- plays fifo file.
 * Returns:
 *			void
 * Side-Effects:
 *
 * Description: Places the dead piece in a new empty position. If there are
 * 				no empty positions, it leaves the piece where it is.
 ******************************************************************************/
void player_eats_player(board_piece** board, int row, int col, int x, int y, int prev_x, int prev_y, int suicidal, int ff_fd)
{

	int 	new_row, new_col, n , last_col;
	char 	buffer[BUFF_SIZE], buffer_aux[BUFF_SIZE];

	memset(buffer,' ',BUFF_SIZE*sizeof(char));
	memset(buffer_aux,' ',BUFF_SIZE*sizeof(char));

	/* gets a column at random*/
	srand(time(NULL));
	new_col = rand()%col;
	last_col = new_col;

	pthread_mutex_lock(&lock_empty);
	if (status.empty == 0) 									{pthread_mutex_unlock(&lock_empty); return;}
	pthread_mutex_unlock(&lock_empty);

	while(1)
	{
		/* tries to lock col*/
		if (!pthread_mutex_trylock(&lock_col[new_col]))
		{
			/* tries to get a random row for n-rows attempts */
			/* getting the right row - if there is any - can be seen as a bernoulli distribution with p = 1/n */
			/* therefore expected number of trials until sucess is ~ 1/p  = n */
			/* if it doesn't find a empty row within the n attempts it tries a new col*/
			for (n = 0; n < row ; n++)
			{

				new_row = rand()%row;

				if(is_empty(new_row, new_col,board))
				{

					/* pacman suicides into monster */
					if (suicidal)
					{
						/* moves pacman */
						move_and_clear(board, prev_x, prev_y, new_row, new_col);
					}

					/* monster hunts pacman*/
					else 
					{
						/* moves pacman */
						move(board,x, y, new_row, new_col);

						/* moves monster and clears position*/
						move_and_clear(board,prev_x, prev_y, x, y);
						
					}
					

					n = sprintf(buffer, "PT %s\n" , print_piece(board, x, y, buffer_aux));
					buffer[n] = '\0';
					write_play_to_main(buffer,ff_fd);

					n = sprintf(buffer, "PT %s\n", print_piece(board, new_row, new_col,buffer_aux));
					buffer[n] = '\0';
					write_play_to_main(buffer,ff_fd);

					n = sprintf(buffer, "CL %d:%d\n", prev_x, prev_y);
					buffer[n] = '\0';
					write_play_to_main(buffer,ff_fd);

					/* unlocks col*/
					pthread_mutex_unlock(&lock_col[new_col]);

					return;
				}
			}

			/* unlocks col*/
			pthread_mutex_unlock(&lock_col[new_col]);
		}

		/*changes col - doesn't repeat last col*/
		while(new_col==last_col)
		{
			last_col = new_col;
			new_col = rand()%col;
		}
	}
}
