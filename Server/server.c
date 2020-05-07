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

#include "server.h"


client* clients_head;

/* decrements by 4 for each user - 2 for pacman and monters, 2 for the extra fruits */
board_info status;


void * client_thread (void* arg)
{
	
	int  	fd;

	fd = *((int*) arg);

	if (client_setup(fd) == -1)
	{
		close(fd);
		remove_client(clients_head,(unsigned long)pthread_self);
		return NULL;
	}





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
    tv.tv_sec =1;
    tv.tv_usec = 0;

    /* allocs memory for struct */
	res = malloc(sizeof(struct addrinfo*));

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



int client_setup(int fd)
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

	srand(time(NULL));
	new_row = rand()%status.row;
	new_col = rand()%status.col;



	while(!is_empty(new_row, new_col,status.board))
	{
		srand(time(NULL));
		new_row = rand()%status.row;
		new_col = rand()%status.col;
	}


	/*create pacman  characters in board*/

	place_piece(status.board,PACMAN, new_row, new_col, (unsigned long) pthread_self(), pac_rgb[0], pac_rgb[1], pac_rgb[2]);


	srand(time(NULL));
	new_row = rand()%status.row;
	new_col = rand()%status.col;


	while(!is_empty(new_row, new_col,status.board))
	{
		srand(time(NULL));
		new_row = rand()%status.row;
		new_col = rand()%status.col;
	}

	
	place_piece(status.board,MONSTER, new_row, new_col, (unsigned long) pthread_self(), mon_rgb[0], mon_rgb[1], mon_rgb[2]);

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


int client_loop(int fd)
{

	int 		n, piece, prev_x, prev_y, x, y;

	char 		buffer[BUFF_SIZE];
	client* 	my_client;



	my_client = search_client(clients_head, (unsigned long) pthread_self);


	while(1)
	{
		if( (n = recv(fd, buffer, BUFF_SIZE, 0)  ) != -1 ) 		{func_err("recv"); return -1;}

		buffer[n] = '\0';

		if(strstr(buffer, "MV"))
		{
			sscanf(buffer, "%*s %d @ %d:%d => %d:%d", &piece, &prev_x, &prev_y, &x,&y);


			/* server will report the error but not exit execution if protocol is followed but values are invalid */
			if ( (piece != PACMAN ) && (piece != MONSTER )) 				fprintf(stderr,"\nInvalid Piece received from user %lx\n", pthread_self());

			if ( (x > status.row) || (y > status.col) )						fprintf(stderr, "\nInvalid Position received from user %lx\n", pthread_self());

			if ( piece_is_correct(x,y,piece, pthread_self(),status.board))	fprintf(stderr, "\nInvalid Last Position received from user %lx\n", pthread_self());


			if (is_empty(x, y, status.board))
			{
				write_play_to_main();
			}

			if (is_brick(x,y,status.board))
			{
				/*horizontal movement*/

				if (prev_x == x)
				{
					

					if (is_empty(x, prev_y + (prev_y - y), status.board))
					{
						/* bounces to x:prevy + (prev_y - y)*/
						write_play_to_main();
					}

					if (is_brick(x, prev_y + (prev_y - y), status.board))
					{

						/* resets counter ? */
						continue;
					}
				}

				/*vertical movement*/

				else if (prev_y == y)
				{
					
					if (is_empty(prev_x + (prev_x - x),y, status.board) )
					{
						/* bounces to x:prevy + (prev_y - y)*/
						write_play_to_main();
					}
					if (is_brick(prev_x + (prev_x - x),y, status.board) )
					{
						/* resets counter ? */
						continue;
					}
				}
				
			}

			if (is_pacman(x,y,status.board))
			{

			}

			if( is_monster(x,y,status.board))
			{

			}
		}

		else if(strstr(buffer, "DC"))							return -1;

		else													{inv_msg(); return -1;}

	}


}



void write_play_to_main()
{
	return;
}

