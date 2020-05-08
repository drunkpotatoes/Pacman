#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <sys/un.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>

#include "server.h"



client* clients_head;

/* decrements by 4 for each user - 2 for pacman and monters, 2 for the extra fruits */
board_info status;


void * client_thread (void* arg)
{
	
	int  	i, fd;

	int 	coord[4];

	/* initializes last coordenates at -1*/
	for (i = 0; i < 4; i++) 	coord[i] = -1;


	fd = *((int*) arg);

	if (client_setup(fd,coord) == -1) 		client_disconnect(fd,coord);

	if (client_loop(fd, coord) == -1)		client_disconnect(fd,coord);




	close(fd);


	return NULL;
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


    /* sets time struct to 1 second */
    tv.tv_sec  = SECONDS_TIMEOUT;
    tv.tv_usec = USECONDS_TIMEOUT;

    /* allocs memory for struct */
	if ( (res = malloc(sizeof(struct addrinfo*)) ) == NULL ) 		mem_err("Addrinfo Struct");

	if ( (fd = server_open(res, SERVER_PORT) )== -1)
	{
		return NULL;
	}

	/* increments port number*/

	while(1)
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
			pthread_create(&thread_id , NULL, client_thread, (void*) &newfd);

		}


	}

	/* closes connection */
	close_connection(res,fd);


	return NULL;
}


int main(int argc, char** argv)
{

	
    pthread_t 	thread_id;
   
	status = init_board();

	clients_head = NULL;

	pthread_create(&thread_id , NULL, accept_thread, NULL);

	getchar();


	return 0;
}


int client_setup(int fd, int* coord)
{	

	int 	n,i,j,num_pieces,new_row, new_col;

	int 	pac_rgb[3];
	int 	mon_rgb[3];
	

	char 	buffer[BUFF_SIZE];
	char 	buffer_aux[PIECE_SIZE];

	/*****************************************/
		/******* needs read protected zone******/
		/***************************************/
	if (status.empty >= 4)
	{
		n = sprintf(buffer,"WE\n");

		buffer[n] = '\0';

		printf("%s\n", buffer);

		if (send(fd,buffer,BUFF_SIZE,0) == -1) 			{func_err("send"); return -1;}

		/*****************************************/
		/******* needs write protected zone******/
		/***************************************/
		status.empty -= 4;

	}

	else	/* all seats taken */
	{

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

	if (strstr(buffer, "CC") == NULL) 					{inv_msg(); return -1;}

	

	if ( sscanf(buffer, "%*s [%d,%d,%d] [%d,%d,%d]\n", &pac_rgb[0], &pac_rgb[1], &pac_rgb[2], &mon_rgb[0], &mon_rgb[1], &mon_rgb[2] ) != 6) 	{func_err("sscanf"); return -1;}


	add_client(&clients_head, (unsigned long) pthread_self(), pac_rgb, mon_rgb);

	/*create pacman and monster characters*/


	get_randoom_position(status.board, status.row, status.col, &new_row, &new_col);

	/*create pacman  characters in board*/

	place_piece(status.board,PACMAN, new_row, new_col, (unsigned long) pthread_self(), pac_rgb[0], pac_rgb[1], pac_rgb[2]);

	coord[0] = new_row;
	coord[1] = new_col;


	get_randoom_position(status.board, status.row, status.col, &new_row, &new_col);

	
	place_piece(status.board,MONSTER, new_row, new_col, (unsigned long) pthread_self(), mon_rgb[0], mon_rgb[1], mon_rgb[2]);

	coord[2] = new_row;
	coord[3] = new_col;

	/* non empty pieces */

	num_pieces = status.row*status.col - status.empty;

	n = sprintf(buffer, "MP %d:%d\n", status.row, status.col);
	buffer[n] = '\0';

	printf("%s\n", buffer);

	if (send(fd,buffer,BUFF_SIZE,0) == -1)				{func_err("send"); return -1;}


	num_pieces = 0;

	/*sends all piece information to the client*/
	
	for (i = 0; i < status.row; i++)
	{
		for (j = 0 ; j < status.col; j++)
		{
			if(is_empty(i,j,status.board)) continue;

			

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
	if(strstr(buffer, "OK") == NULL) 					{inv_msg(); return -1;}


	/* setup completed */
	print_clients(clients_head);
	print_board(status.row, status.col,status.board);


	return 0;
}


int client_loop(int fd, int * coord)
{

	int 				n, piece, prev_x, prev_y, x, y;

	char 				buffer[BUFF_SIZE];
	client* 			my_client;

	struct 	timeval 	tv;

	/* sets time struct to 0 seconds  */
    tv.tv_sec  = 0;
    tv.tv_usec = 0;


	memset(buffer,'\0',BUFF_SIZE);

	my_client = search_client(clients_head, (unsigned long) pthread_self);

	/* sets a timeout of 0 second => removes timeout*/
	setsockopt(fd,SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

	while(1)
	{
		if( (n = recv(fd, buffer, BUFF_SIZE, 0)  ) == -1 ) 		{func_err("recv"); fprintf(stderr, "recv : %s (%d)\n", strerror(errno), errno); return -1;}

		/* 0 bytes received*/
		if (n == 0) continue;


		/* LOCK FIFO */

		/* NEED TO IMPLEMENT*/

		buffer[n] = '\0';

		printf("Received: %s\n", buffer);

		if(strstr(buffer, "MV") != NULL)
		{
			if ( sscanf(buffer, "%*s %d @ %d:%d => %d:%d", &piece, &prev_x, &prev_y, &x,&y) != 5) 	{fprintf(stderr, "\nInvalid Format received from user %lx\n", pthread_self()); return -1;}

			/* server will report the error but not exit execution if protocol is followed but values are invalid */
			if ( (piece != PACMAN ) && (piece != MONSTER )) 										fprintf(stderr,"\nInvalid Piece received from user %lx\n", pthread_self());

			if ( (x > status.row) || (y > status.col) )												fprintf(stderr, "\nInvalid Position received from user %lx\n", pthread_self());

			if ( piece_is_correct(x,y,piece, pthread_self(),status.board))							fprintf(stderr, "\nInvalid Last Position received from user %lx\n", pthread_self());
			

			if ( piece == PACMAN) 	

				if (pacman_movement(x,y,prev_x,prev_y,coord,my_client) == -1) 						fprintf(stderr, "\nUnexpected movement received from user %lx", pthread_self());

			if ( piece == MONSTER)	

				if (monster_movement(x,y,prev_x,prev_y,coord,my_client) == -1) 						fprintf(stderr, "\nUnexpected movement received from user %lx", pthread_self());
		}

		else if(strstr(buffer, "DC"))							return -1;

		else													{inv_msg();}

	}
}


void client_disconnect(int fd, int* coord)
{
	int 	n;
	char 	buffer[BUFF_SIZE];

	/* closes socket */
	close(fd);

	/* removes client */
	remove_client(&clients_head,(unsigned long)pthread_self);

	/* cleans pacman and monster from board */

	if(coord[0] != -1 && coord[1] != -1)
	{
		clear_place(status.board, coord[0], coord[1]);
		n = sprintf(buffer, "CL %d:%d", coord[0], coord[1]);
		buffer[n] = '\0';
		write_play_to_main(buffer);
	} 	
	if(coord[2] != -1 && coord[3] != -1)
	{
		clear_place(status.board, coord[2], coord[3]);
		n = sprintf(buffer, "CL %d:%d", coord[2], coord[3]);
		buffer[n] = '\0';
		write_play_to_main(buffer);
	}	
	
	return;
}


void write_play_to_main(char* play)
{
	if (play == NULL)			return;
	
	printf("%s\n", play);

	return;
}


int pacman_movement(int x, int y, int prev_x , int prev_y, int* coord, client* my_client)
{
	int 	n, new_x, new_y;
	int 	rgb[3];

	char 	buffer[BUFF_SIZE];
	char 	buffer_aux[PIECE_SIZE];


	get_pac_rgb(my_client,rgb);


	/* next position is empty */
	if (is_empty(x, y, status.board))
	{
		/* moves pacman */	
		move_and_clear(status.board,prev_x, prev_y, x, y);

		n = sprintf(buffer, "PT %d @ %d:%d [%d,%d,%d]\n", PACMAN, x,y, rgb[0], rgb[1], rgb[2]);
		buffer[n] = '\0';
		write_play_to_main(buffer);

		n = sprintf(buffer, "CL %d:%d", prev_x, prev_y);
		buffer[n] = '\0';
		write_play_to_main(buffer);

		/*updates pacman last coord*/
		coord[0] = x;
		coord[1] = y;

		return 0;
	}

	/* next position is a brick */
	if (is_brick(x,y,status.board))
	{
		/*horizontal movement*/
		if (prev_x == x)
		{
			/* gets target y for the bounce */		
			new_y = prev_y + (prev_y-y);

			/* bounce to empty place */
			if (is_empty(x, new_y, status.board))
			{
				/* moves pacman */
				move_and_clear(status.board,prev_x, prev_y, x, new_y);

				n = sprintf(buffer, "PT %d @ %d:%d [%d,%d,%d]\n", PACMAN, x,new_y, rgb[0], rgb[1], rgb[2]);
				buffer[n] = '\0';
				write_play_to_main(buffer);

				sprintf(buffer, "CL %d:%d", prev_x, prev_y);
				buffer[n] = '\0';
				write_play_to_main(buffer);

				/*updates pacman last coord*/
				coord[0] = x;
				coord[1] = new_y;

				return 0;
			}

			/* bounce agains brick */
			if (is_brick(x,new_y, status.board))
			{

				/*pacman stays were it is*/

				return 0;
			}

			/* bounce to object, updates y*/
			else
			{
				y = new_y;
			}
		}

		/*vertical movement*/
		else if (prev_y == y)
		{
			/* gets target x for the bounce */	
			new_x = prev_x + (prev_x - x);

			/* bounce to empty place */
			if (is_empty(new_x,y, status.board) )
			{
				/* moves pacman */
				move_and_clear(status.board,prev_x, prev_y, new_x, y);

				n = sprintf(buffer, "PT %d @ %d:%d [%d,%d,%d]\n", PACMAN, new_x,y, rgb[0], rgb[1], rgb[2]);
				buffer[n] = '\0';
				write_play_to_main(buffer);

				n = sprintf(buffer, "CL %d:%d", prev_x, prev_y);
				buffer[n] = '\0';
				write_play_to_main(buffer);

				/*updates pacman last coord*/
				coord[0] = new_x;
				coord[1] = y;


				return 0;
			}

			/* bounce agains a wall */
			if (is_brick(new_x,y, status.board) )
			{

				/*pacman stays were it is*/
				return 0;
			}

			/* bounce to object, updates x */
			else
			{
				new_x = x;
			}
		}		
	}

	/* hits enemy pacman */
	if (is_pacman(x,y,status.board))
	{
		/* characters change positions */
		switch_pieces(status.board,prev_x, x, prev_y, y);

		n = sprintf(buffer, "PT %d @ %d:%d [%d,%d,%d]\n", PACMAN, x,y, rgb[0], rgb[1], rgb[2]);
		buffer[n] = '\0';
		write_play_to_main(buffer);

		n = sprintf(buffer, "PT %s", print_piece(status.board, prev_x, prev_y,buffer_aux));
		buffer[n] = '\0';
		write_play_to_main(buffer);

		/*updates pacman last coord*/
		coord[0] = x;
		coord[1] = y;

		return 0;
	}

	/* hits monster*/
	if( is_monster(x,y,status.board))
	{

		/* friendly monster*/
		if(get_id(status.board, x, y) == pthread_self())
		{
			/* pacman and monster change positions*/
			switch_pieces(status.board,prev_x, x, prev_y, y);

			n = sprintf(buffer, "PT %d @ %d:%d [%d,%d,%d]\n", PACMAN, x,y, rgb[0], rgb[1], rgb[2]);
			buffer[n] = '\0';
			write_play_to_main(buffer);

			n = sprintf(buffer, "PT %s", print_piece(status.board, prev_x, prev_y,buffer_aux));
			buffer[n] = '\0';
			write_play_to_main(buffer);

			/*updates pacman last coord*/
			coord[0] = x;
			coord[1] = y;

			return 0;
		}

		/* enemy monster*/
		else
		{
			/* pacman gets killed */

			/* get next pacman position*/
			get_randoom_position(status.board, status.row, status.col, &new_x, &new_y);

			/* moves pacman */
			move_and_clear(status.board,prev_x, prev_y, new_x, new_y);

			n = sprintf(buffer, "PT %d @ %d:%d [%d,%d,%d]\n", PACMAN, new_x,new_y, rgb[0], rgb[1], rgb[2]);
			buffer[n] = '\0';
			write_play_to_main(buffer);

			n = sprintf(buffer, "CL %d:%d", prev_x, prev_y);
			buffer[n] = '\0';
			write_play_to_main(buffer);

			/*updates pacman last coord*/
			coord[0] = new_x;
			coord[1] = new_y;

			return 0;


		}
	}

	return -1;
}

int monster_movement(int x, int y, int prev_x , int prev_y,int* coord, client* my_client)
{
	int 	n, new_x, new_y;
	int 	rgb[3];

	char 	buffer[BUFF_SIZE];
	char 	buffer_aux[PIECE_SIZE];



	get_mon_rgb(my_client,rgb);


	/* next position is empty */
	if (is_empty(x, y, status.board))
	{
		/* moves monster */	
		move_and_clear(status.board,prev_x, prev_y, x, y);

		n = sprintf(buffer, "PT %d @ %d:%d [%d,%d,%d]\n", MONSTER, x,y, rgb[0], rgb[1], rgb[2]);
		buffer[n] = '\0';
		write_play_to_main(buffer);

		n = sprintf(buffer, "CL %d:%d", prev_x, prev_y);
		buffer[n] = '\0';
		write_play_to_main(buffer);

		coord[2] = x;
		coord[3] = y;

		return 0;
	}

	/* next position is a brick */
	if (is_brick(x,y,status.board))
	{
		/*horizontal movement*/
		if (prev_x == x)
		{
			/* gets target y for the bounce */		
			new_y = prev_y + (prev_y-y);

			/* bounce to empty place */
			if (is_empty(x, new_y, status.board))
			{
				/* moves pacman */
				move_and_clear(status.board,prev_x, prev_y, x, new_y);

				n = sprintf(buffer, "PT %d @ %d:%d [%d,%d,%d]\n", PACMAN, x,new_y, rgb[0], rgb[1], rgb[2]);
				buffer[n] = '\0';
				write_play_to_main(buffer);

				n = sprintf(buffer, "CL %d:%d", prev_x, prev_y);
				buffer[n] = '\0';
				write_play_to_main(buffer);

				coord[2] = x;
				coord[3] = new_y;

				return 0;
			}

			/* bounce agains brick */
			if (is_brick(x,new_y, status.board))
			{

				/*pacman stays were it is*/

				return 0;
			}

			/* bounce to object, updates y*/
			else
			{
				y = new_y;
			}
		}

		/*vertical movement*/
		else if (prev_y == y)
		{
			/* gets target x for the bounce */	
			new_x = prev_x + (prev_x - x);

			/* bounce to empty place */
			if (is_empty(new_x,y, status.board) )
			{
				/* moves monster */
				move_and_clear(status.board,prev_x, prev_y, new_x, y);

				n = sprintf(buffer, "PT %d @ %d:%d [%d,%d,%d]\n", PACMAN, new_x,y, rgb[0], rgb[1], rgb[2]);
				buffer[n] = '\0';
				write_play_to_main(buffer);

				n = sprintf(buffer, "CL %d:%d", prev_x, prev_y);
				buffer[n] = '\0';
				write_play_to_main(buffer);

				coord[2] = new_x;
				coord[3] = y;

				return 0;
			}

			/* bounce agains a wall */
			if (is_brick(new_x,y, status.board) )
			{

				/*monster stays were it is*/
				return 0;
			}

			/* bounce to object, updates x */
			else
			{
				new_x = x;
			}
		}		
	}

	/* hits enemy monster */
	if (is_monster(x,y,status.board))
	{
		/* characters change positions */
		switch_pieces(status.board,prev_x, x, prev_y, y);

		n = sprintf(buffer, "PT %d @ %d:%d [%d,%d,%d]\n", MONSTER, x,y, rgb[0], rgb[1], rgb[2]);
		buffer[n] = '\0';
		write_play_to_main(buffer);

		n = sprintf(buffer, "PT %s", print_piece(status.board, prev_x, prev_y,buffer_aux));
		buffer[n] = '\0';

		write_play_to_main(buffer);

		coord[2] = x;
		coord[3] = y;

		return 0;
	}

	/* hits monster*/
	if( is_pacman(x,y,status.board))
	{

		/* friendly pacman*/
		if(get_id(status.board, x, y) == pthread_self())
		{
			/* pacman and monster change positions*/
			switch_pieces(status.board,prev_x, x, prev_y, y);	

			n = sprintf(buffer, "PT %d @ %d:%d [%d,%d,%d]\n", MONSTER, x,y, rgb[0], rgb[1], rgb[2]);
			buffer[n] = '\0';
			write_play_to_main(buffer);


			n = sprintf(buffer, "PT %s", print_piece(status.board, prev_x, prev_y,buffer_aux));
			buffer[n] = '\0';

			write_play_to_main(buffer);

			coord[2] = x;
			coord[3] = y;

			return 0;
		}

		/* enemy pacman*/
		else
		{
			/* pacman gets killed */

			/* get next pacman position */
			get_randoom_position(status.board, status.row, status.col, &new_x, &new_y);

			/* moves pacman */
			move(status.board,x, y, new_x, new_y);

			/* moves monster and clears position*/
			move_and_clear(status.board,prev_x, prev_y, x, y);

			n = sprintf(buffer, "PT %d @ %d:%d [%d,%d,%d]\n", MONSTER, x,y, rgb[0], rgb[1], rgb[2]);
			buffer[n] = '\0';
			write_play_to_main(buffer);


			n = sprintf(buffer, "PT %s", print_piece(status.board, new_x, new_y,buffer_aux));
			buffer[n] = '\0';
			write_play_to_main(buffer);

			n = sprintf(buffer, "CL %d:%d", prev_x, prev_y);
			buffer[n] = '\0';
			write_play_to_main(buffer);

			coord[2] = x;
			coord[3] = y;
			
			return 0;


		}
	}

	return -1;
}




