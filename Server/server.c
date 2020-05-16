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



client* clients_head;

board_info status;

/*typedef struct board_infos{

    board_piece** board;
    int   row;
    int   col;
    int   empty;

}board_info; */

pthread_mutex_t * lock_col;

pthread_rwlock_t  lock_clients;

pthread_mutex_t   lock_empty;

int shut_down;

void handler (int sigtype)
{
	shut_down = 1;
}


int main(int argc, char** argv)
{

	int 		i;
    pthread_t 	thread_id;
   	struct 		sigaction action_sig_pipe;
   	struct 		sigaction action_int;


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

	status = init_board();

	clients_head = NULL;

	shut_down = 0;


	/* creates lock for each col*/
	if ( (lock_col = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t)*status.col) ) == NULL) 			  mem_err("lock_col");

	/* initializes read-write lock for clients*/
	if (pthread_rwlock_init(&lock_clients,NULL) != 0)								{func_err("pthread_rwlock_init"); exit(1);}

	/* initializes lock for fifo*/
	if (pthread_mutex_init(&lock_empty,NULL) != 0)									{func_err("pthread_mutex_init"); exit(1);}

	/* initializes lock for each col*/
	for (i = 0; i < status.col ; i++)
		if (pthread_mutex_init(&lock_col[i],NULL) != 0)								{func_err("pthread_mutex_init"); exit(1);}


	/* creates thread to accept clients*/
	if (pthread_create(&thread_id , NULL, accept_thread, NULL)) 					{func_err("pthread_create"); exit(1);}

	if (pthread_detach(thread_id))													{func_err("pthread_detach"); exit(1);}
	/* thread to send information to all clients*/
	main_thread();

	/* waits for all threads to clean up*/
	sleep(2);

	server_disconnect();

	return 0;
}

void * accept_thread (void* arg)
{
	
	int 					fd,newfd,n;

	char 					buffer[BUFF_SIZE];

	struct sockaddr_in 		addr;
	struct addrinfo 		**res;
	struct timeval 			tv;

	socklen_t 				addrlen;
    
    pthread_t 				thread_id;



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

	/* increments port number*/

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


		/* prints message */
		printf("%s\n", buffer);

		/* connection request*/	
		if(strstr(buffer, "RQ") != NULL)
		{
			/* creates thread to handle client */
			if (pthread_create(&thread_id , NULL, client_thread, (void*) &newfd)) 	func_err("pthread_create");
			if (pthread_detach(thread_id))											func_err("pthread_detach");

		}
	}

	/* closes connection */
	close_connection(res,fd);


	return NULL;
}


void * client_thread (void* arg)
{
	int  	fd, ff_fd;

	fd = *((int*) arg);


	/* initializes fifo file with -1*/
	ff_fd = -1;


	if (client_setup(fd,&ff_fd) == -1) 			{client_disconnect(fd,ff_fd); return NULL;}

	if (client_loop(fd,ff_fd) == -1)			{client_disconnect(fd,ff_fd); return NULL;}

	client_disconnect(fd,ff_fd);

	return NULL;
}

int client_setup(int fd, int* pff_fd)
{	

	int 	n,i,j,num_pieces;

	int 	pac_rgb[3];
	int 	mon_rgb[3];
	

	char 	buffer[BUFF_SIZE];
	char 	buffer_aux[PIECE_SIZE];

	memset(buffer,' ',BUFF_SIZE*sizeof(char));


	/* tries to opens fifo */
	if ((*pff_fd = open(FIFO_FILE, O_WRONLY))== -1) 		
	{
		/*if it fails to open, tries to create */
		if (mkfifo(FIFO_FILE, 0666)!=0)						{func_err("mkfifo"); return -1; }

		/* tries to open again */
		if ((*pff_fd = open(FIFO_FILE, O_WRONLY))== -1)		{func_err("open");   return -1; }
	}

	printf("FIFO OPEN FOR WRITE\n");

	pthread_mutex_lock(&lock_empty);

	if (status.empty >= 4)
	{
		n = sprintf(buffer,"WE # %lx\n", pthread_self());

		buffer[n] = '\0';

		printf("%s\n", buffer);

		if (send(fd,buffer,BUFF_SIZE,0) == -1) 			{func_err("send"); return -1;}

		/* decrements by 4 for each user - 2 for pacman and monters, 2 for the extra fruits */
		status.empty -= 4;

		pthread_mutex_unlock(&lock_empty);

	}

	else	/* all seats taken */
	{
		pthread_mutex_unlock(&lock_empty);

		/* notifies user */
		n = sprintf(buffer,"W8\n");
		buffer[n] = '\0';

		printf("%s\n", buffer);

		if (send(fd,buffer,BUFF_SIZE,0) == -1 ) 		{func_err("send"); return -1;}

		return -1;
	}


	if ( (n = recv(fd, buffer, BUFF_SIZE, 0) )== -1) 	{func_err("recv"); return -1;}
	buffer[n] = '\0';

	printf("%s\n", buffer);

	if (strstr(buffer, "CC") == NULL) 					{inv_msg(buffer); return -1;}

	

	if ( sscanf(buffer, "%*s [%d,%d,%d] [%d,%d,%d]\n", &pac_rgb[0], &pac_rgb[1], &pac_rgb[2], &mon_rgb[0], &mon_rgb[1], &mon_rgb[2] ) != 6) 	{inv_format(buffer); return -1;}

	/*places pacman and monster in randoom positions*/

	place_randoom_position(status.board, status.row, status.col, PACMAN,  pac_rgb, *pff_fd);
	place_randoom_position(status.board, status.row, status.col, MONSTER,  mon_rgb, *pff_fd);

	/* non empty pieces */

	num_pieces = status.row*status.col - status.empty;

	n = sprintf(buffer, "MP %d:%d\n", status.row, status.col);
	buffer[n] = '\0';

	printf("%s\n", buffer);

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

			printf("%s\n",buffer);
			num_pieces++;
		}
	}


	/* sends summary of sent data */
	n = sprintf(buffer, "SS %d\n", num_pieces);
	buffer[n] = '\0';

	if (send(fd,buffer,BUFF_SIZE, 0) == -1) 			{func_err("send"); return -1;}

	printf("%s\n",buffer);


	/* waits for client to answer*/
	if ( (n = recv(fd, buffer,BUFF_SIZE,0) ) == -1) 	{func_err("recv"); return -1;}


	/* cleans buffer */
	buffer[n] = '\0';


	/* prints message */
	printf("%s\n", buffer);

	/* connection request*/	
	if(strstr(buffer, "OK") == NULL) 					{inv_msg(buffer); return -1;}


	/* setup completed */
	print_clients(clients_head);
	print_board(status.row, status.col,status.board);


	return 0;
}


int client_loop(int fd, int ff_fd)
{

	int 				n, piece, prev_x, prev_y, x, y, new_y, new_x;
	int 				pac_rgb[3], mon_rgb[3], coord[4];
	char 				buffer[BUFF_SIZE];
	struct 	timeval 	tv, start, end;

	double 				diff;



	/* sets time struct to 1 seconds  */
    tv.tv_sec  = 1;
    tv.tv_usec = 0;


	memset(buffer,' ',BUFF_SIZE*sizeof(char));

	
	/* sets a timeout of 0 second => removes timeout*/
	setsockopt(fd,SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);


	/* locks clients to read only*/
	pthread_rwlock_rdlock(&lock_clients);

	get_pac_rgb(clients_head,(unsigned long)pthread_self(),&pac_rgb[0],&pac_rgb[1] ,&pac_rgb[2]);

	get_mon_rgb(clients_head,(unsigned long)pthread_self(),&mon_rgb[0],&mon_rgb[1], &mon_rgb[2]);

	/*unlocks*/
	pthread_rwlock_unlock(&lock_clients);


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


			/* gets current time*/
			gettimeofday(&end,NULL);

			/* calculates how long the user has been inative*/
			diff = (double) (end.tv_usec - start.tv_usec) + (double) (end.tv_sec - start.tv_sec)*1000000;


			/*user inative for too long*/
			if (diff >= USER_TIMEOUT_USECONDS)
			{

				for (n = 0; n < status.col ; n++) 			pthread_mutex_lock(&lock_col[n]);
				/* cleans pacman and monster from board */
				clear_player(status.board, status.row, status.col, (unsigned long) pthread_self(), coord);

				for (n = 0; n < status.col ; n++) 			pthread_mutex_unlock(&lock_col[n]);

				/* generates new positions and clears hold ones*/
				if(coord[0] != -1 && coord[1] != -1 && coord[2] != -1 && coord[3] != -1)
				{
					
					n = sprintf(buffer, "CL %d:%d", coord[2], coord[3]);
					buffer[n] = '\0';
					write_play_to_main(buffer,ff_fd);

					n = sprintf(buffer, "CL %d:%d", coord[0], coord[1]);
					buffer[n] = '\0';
					write_play_to_main(buffer,ff_fd);

					place_randoom_position(status.board, status.row, status.col, PACMAN,  pac_rgb, ff_fd);
					place_randoom_position(status.board, status.row, status.col, MONSTER,  mon_rgb, ff_fd);
				}
				/*updates clock*/
				gettimeofday(&start,NULL);
			}

			continue;
		}
		

		buffer[n] = '\0';

		printf("Received: %s\n", buffer);

		gettimeofday(&end,NULL);

		diff = (double) (end.tv_usec - start.tv_usec) + (double) (end.tv_sec - start.tv_sec)*1000000;

		if(strstr(buffer, "MV") != NULL && ( diff >= USER_MAX_TIME_USECONDS ))
		{
			if ( sscanf(buffer, "%*s %d @ %d:%d => %d:%d", &piece, &prev_x, &prev_y, &x,&y) != 5) 					{inv_format(buffer); continue;}

			/* out of boundaries*/
			if ( (x > status.row) || (y > status.col) || x < -1 || y < -1 )											continue;

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
				if (!pacman_movement(status.board, status.row, status.col,x,y,prev_x,prev_y,ff_fd,fd, pac_rgb))  	continue;

				/*updates clock*/
				gettimeofday(&start,NULL);

			}

			/* monster movement */
			else if ( piece == MONSTER)	
			{

				if (!monster_movement(status.board, status.row, status.col,x,y,prev_x,prev_y,ff_fd,fd, mon_rgb)) 	continue;

				/*updates clock*/
				gettimeofday(&start,NULL);

			}

			else 		 																							inv_piece(piece);

		}

		else if(strstr(buffer, "DC"))							return -1;

		else													{inv_msg(buffer);}
	}


	return -1;
}


void client_disconnect(int fd, int ff_fd)
{
	int 	n;
	char 	buffer[BUFF_SIZE];
	int 	coord[4];

	memset(buffer,' ',BUFF_SIZE*sizeof(char));
	memset(coord, -1, 4*sizeof(int));

	

	/* closes socket */
	if (fd != -1 ) 			
	{	
		/* sends dc message*/
		n = sprintf(buffer, "DC\n");
		buffer[n] = '\0';
		send(fd,buffer, BUFF_SIZE,0);

		close(fd); 	
	}


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
		
		n = sprintf(buffer, "CL %d:%d", coord[0], coord[1]);
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);
	} 	
	if(coord[2] != -1 && coord[3] != -1)
	{
		clear_place(status.board, coord[2], coord[3]);
		n = sprintf(buffer, "CL %d:%d", coord[2], coord[3]);
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);
	}	


	/* closes fifo */
	if (fd != -1 ) 		close(ff_fd);
}


int write_play_to_main(char* play, int fd)
{


	if (play == NULL)									return -1;
	
	printf("T: %s\n", play);

	/* writes to fifo*/
	if (write(fd, play, BUFF_SIZE) == -1) 				{func_err("write"); return -1;}
	
	return 0;
}


int main_thread()
{


	int 	n, fd;
	char 	buffer[BUFF_SIZE];


	/* tries to open the fifo */
	if ( (fd = open(FIFO_FILE, O_RDONLY) ) == -1)
	{
		/* if it fails to open, tries to create */
		if (mkfifo(FIFO_FILE, 0666)!=0)					{func_err("mkfifo"); return -1; }

		/* tries to open again */
		if ( (fd = open(FIFO_FILE, O_RDONLY) ) == -1)	{func_err("open");   return -1; }
	}


	printf("FIFO OPEN FOR READ\n");
	

	/* main cycle */
	while(!shut_down)
	{

		if (  (n = read(fd, buffer, BUFF_SIZE) ) == -1 ) 				continue; 

		if (n == 0)														continue;


		buffer[n] = '\0';

		printf("\n\tM: %s\n", buffer);

		if (send_all_clients(clients_head,buffer,BUFF_SIZE) == -1) 	{fprintf(stderr, "No clients :(\n");continue;}

	}


	sleep(1);
	close(fd);

	return 0;
}


void server_disconnect()
{
	int i;

	for (i =0; i < status.col ; i++) 	pthread_mutex_lock(&lock_col[i]);
	free_board(status.row, status.board);
	status.board = NULL;
	for (i =0; i < status.col; i++) 	pthread_mutex_unlock(&lock_col[i]);

	pthread_rwlock_wrlock(&lock_clients);
	free_clients(clients_head);
	clients_head = NULL;
	pthread_rwlock_unlock(&lock_clients);

	for(i = 0; i < status.col ; i++)
		if (pthread_mutex_destroy(&lock_col[i])) 			func_err("pthread_mutex_destroy");
	
	/* frees mutexes */
	free(lock_col);
	
	if (pthread_rwlock_destroy(&lock_clients)) 				func_err("pthread_mutex_destroy");
	if (pthread_mutex_destroy(&lock_empty)) 				func_err("pthread_mutex_destroy");

	
}


int pacman_movement(board_piece** board,int row, int col, int x, int y, int prev_x , int prev_y, int ff_fd, int fd, int* rgb)
{
	int 	n, new_x, new_y;

	char 	buffer[BUFF_SIZE];
	char 	buffer_aux[PIECE_SIZE];

	memset(buffer,' ',BUFF_SIZE*sizeof(char));
		
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
	 * when finding the new randoom position, to avoid possible deadlocks*/

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

		n = sprintf(buffer, "PT %d @ %d:%d [%d,%d,%d] # %lx\n", PACMAN, x,y, rgb[0], rgb[1], rgb[2], pthread_self());
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);

		n = sprintf(buffer, "CL %d:%d\n", prev_x, prev_y);
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);

	}

	/* hits enemy pacman */
	else if (is_pacman(x,y,board))
	{
		/* characters change positions */
		switch_pieces(board,prev_x, x, prev_y, y);

		n = sprintf(buffer, "PT %d @ %d:%d [%d,%d,%d] # %lx\n", PACMAN, x,y,rgb[0], rgb[1], rgb[2], pthread_self());
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);

		n = sprintf(buffer, "PT %s", print_piece(board, prev_x, prev_y,buffer_aux));
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);

	}

	/* hits monster*/
	else if( is_monster(x,y,board))
	{

		/* friendly monster*/
		if(get_id(board, x, y) == pthread_self())
		{
			/* pacman and monster change positions*/
			switch_pieces(board,prev_x, x, prev_y, y);

			n = sprintf(buffer, "PT %d @ %d:%d [%d,%d,%d] # %lx\n", PACMAN, x,y,rgb[0], rgb[1], rgb[2], pthread_self());
			buffer[n] = '\0';
			write_play_to_main(buffer,ff_fd);

			n = sprintf(buffer, "PT %s", print_piece(board, prev_x, prev_y,buffer_aux));
			buffer[n] = '\0';
			write_play_to_main(buffer,ff_fd);

		}

		/* enemy monster*/
		else
		{
			monster_eats_pacman(board, row, col, x, y, prev_x, prev_y, rgb, ff_fd);
		}
	}

	else
	{
		printf("Weird piece\n");

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


	return 1;
}

int monster_movement(board_piece** board, int row, int col, int x, int y, int prev_x , int prev_y, int ff_fd, int fd, int* rgb)
{
	int 	n, new_x, new_y;

	char 	buffer[BUFF_SIZE];
	char 	buffer_aux[PIECE_SIZE];

	memset(buffer,' ',BUFF_SIZE*sizeof(char));


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
	 * when finding the new randoom position, to avoid possible deadlocks*/


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

		n = sprintf(buffer, "PT %d @ %d:%d [%d,%d,%d] # %lx\n", MONSTER, x,y, rgb[0], rgb[1], rgb[2], pthread_self());
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);

		n = sprintf(buffer, "CL %d:%d", prev_x, prev_y);
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);

	}

	/* hits enemy monster */
	else if (is_monster(x,y,board))
	{
		/* characters change positions */
		switch_pieces(board,prev_x, x, prev_y, y);

		n = sprintf(buffer, "PT %d @ %d:%d [%d,%d,%d] # %lx\n", MONSTER, x,y, rgb[0], rgb[1], rgb[2], pthread_self());
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);

		n = sprintf(buffer, "PT %s", print_piece(board, prev_x, prev_y,buffer_aux));
		buffer[n] = '\0';
		write_play_to_main(buffer,ff_fd);

	}

	/* hits pacman*/
	else if( is_pacman(x,y,board))
	{

		/* friendly pacman*/
		if(get_id(board, x, y) == pthread_self())
		{
			/* pacman and monster change positions*/
			switch_pieces(board,prev_x, x, prev_y, y);	

			n = sprintf(buffer, "PT %d @ %d:%d [%d,%d,%d] # %lx\n", MONSTER, x,y, rgb[0], rgb[1], rgb[2], pthread_self());
			buffer[n] = '\0';
			write_play_to_main(buffer,ff_fd);


			n = sprintf(buffer, "PT %s", print_piece(board, prev_x, prev_y,buffer_aux));
			buffer[n] = '\0';
			write_play_to_main(buffer,ff_fd);

		}

		/* enemy pacman*/
		else
		{
			monster_eats_pacman(board, row, col, x, y, prev_x, prev_y, rgb, ff_fd);
		}
	}

	else
	{
		printf("Weird piece\n");

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

	return 1;
}


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

void place_randoom_position(board_piece** board, int row, int col, int piece, int* rgb, int ff_fd)
{
	int 	new_row, new_col, n , last_col;
	char 	buffer[BUFF_SIZE];

	memset(buffer,' ',BUFF_SIZE*sizeof(char));

	/* gets a column at randoom*/
	srand(time(NULL));
	new_col = rand()%col;
	last_col = new_col;

	while(1)
	{
		/* tries to lock col*/
		if (!pthread_mutex_trylock(&lock_col[new_col]))
		{
			/* tries to get a randoom row for n-rows attempts */
			/* getting the right row - if there is any - can be seen as a bernoulli distribution with p = 1/n */
			/* therefore expected number of trials until sucess is ~ 1/p  = n */
			/* if it doesn't find a empty row within the n attempts it tries a new col*/
			for (n = 0; n < row ; n++)
			{
				srand(time(NULL));
				new_row = rand()%row;

				if(is_empty(new_row, new_col,board))
				{

					place_piece(status.board,piece,new_row,new_col,(unsigned long) pthread_self(), rgb[0], rgb[1], rgb[2]);

					n = sprintf(buffer, "PT %d @ %d:%d [%d,%d,%d] # %lx\n", piece, new_row, new_col, rgb[0], rgb[1], rgb[2], pthread_self());
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
			srand(time(NULL));
			last_col = new_col;
			new_col = rand()%col;
		}
	}
}

void monster_eats_pacman(board_piece** board, int row, int col, int x, int y, int prev_x, int prev_y, int* rgb, int ff_fd)
{

	int 	new_row, new_col, n , last_col;
	char 	buffer[BUFF_SIZE], buffer_aux[BUFF_SIZE];

	memset(buffer,' ',BUFF_SIZE*sizeof(char));
	memset(buffer_aux,' ',BUFF_SIZE*sizeof(char));

	/* gets a column at randoom*/
	srand(time(NULL));
	new_col = rand()%col;
	last_col = new_col;

	while(1)
	{
		/* tries to lock col*/
		if (!pthread_mutex_trylock(&lock_col[new_col]))
		{
			/* tries to get a randoom row for n-rows attempts */
			/* getting the right row - if there is any - can be seen as a bernoulli distribution with p = 1/n */
			/* therefore expected number of trials until sucess is ~ 1/p  = n */
			/* if it doesn't find a empty row within the n attempts it tries a new col*/
			for (n = 0; n < row ; n++)
			{
				srand(time(NULL));
				new_row = rand()%row;

				if(is_empty(new_row, new_col,board))
				{

					/* moves pacman */
					move(board,x, y, new_row, new_col);

					/* moves monster and clears position*/
					move_and_clear(board,prev_x, prev_y, x, y);

					n = sprintf(buffer, "PT %d @ %d:%d [%d,%d,%d] # %lx\n", MONSTER, x,y, rgb[0], rgb[1], rgb[2], pthread_self());
					buffer[n] = '\0';
					write_play_to_main(buffer,ff_fd);

					n = sprintf(buffer, "PT %s", print_piece(board, new_col, new_row,buffer_aux));
					buffer[n] = '\0';
					write_play_to_main(buffer,ff_fd);

					n = sprintf(buffer, "CL %d:%d", prev_x, prev_y);
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
			srand(time(NULL));
			last_col = new_col;
			new_col = rand()%col;
		}
	}
}