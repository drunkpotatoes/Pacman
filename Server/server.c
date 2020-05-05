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
	
	int  fd;
	fd = *((int*) arg);

	if (client_setup(fd) == -1)
	{
		close(fd);
		return NULL;
	}


	close(fd);


	return NULL;
}

void * accept_thread (void* arg)
{
	
	int fd, newfd, n;

	char buffer[BUFF_SIZE];

	struct sockaddr_in addr;
	struct addrinfo **res;
	struct timeval tv;

	socklen_t addrlen;
    
    pthread_t thread_id;


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

	
	
    pthread_t thread_id;
   
	status = init_board();

	clients_head = NULL;

	pthread_create(&thread_id , NULL, accept_thread, NULL);

	getchar();


	return 0;
}



int client_setup(int fd)
{	

	int n,i,j,num_pieces;

	int pac_rgb[3];
	int mon_rgb[3];
	
	char buffer[BUFF_SIZE];

	/*****************************************/
		/******* needs read protected zone******/
		/***************************************/
	if (status.empty >= 4)
	{
		n = sprintf(buffer,"WE\n");

		buffer[n] = '\0';

		printf("%s\n", buffer);

		send(fd,buffer,BUFF_SIZE,0);

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

		send(fd,buffer,BUFF_SIZE,0);

		return -1;
	}


	n = recv(fd, buffer, BUFF_SIZE, 0);
	buffer[n] = '\0';

	printf("%s\n", buffer);

	if (strstr(buffer, "CC") == NULL) 		return -1;

	

	sscanf(buffer, "%*s %d,%d,%d %d,%d,%d\n", &pac_rgb[0], &pac_rgb[1], &pac_rgb[2], &mon_rgb[0], &mon_rgb[1], &mon_rgb[2] );


	add_client(&clients_head, (unsigned long) pthread_self(), pac_rgb, mon_rgb);


	/* non empty pieces */

	num_pieces = status.row*status.col - status.empty;

	n = sprintf(buffer, "MP %d:%d\n", status.row, status.col);
	buffer[n] = '\0';

	printf("%s\n", buffer);

	send(fd,buffer,BUFF_SIZE,0);


	num_pieces = 0;

	/*sends all piece information to the client*/
	
	for (i = 0; i < status.row; i++)
	{
		for (j = 0 ; j < status.col; j++)
		{
			if(status.board[i][j].piece == EMPTY) continue;

			n = sprintf(buffer, "PT %d@%d:%d %d,%d,%d\n", status.board[i][j].piece,i,j,status.board[i][j].r, status.board[i][j].g, status.board[i][j].b);
			buffer[n] = '\0';
			send(fd,buffer,BUFF_SIZE, 0);
			printf("%s\n",buffer);
			num_pieces++;
		}
	}


	/* sends summary of sent data */
	n = sprintf(buffer, "SS %d\n", num_pieces);
	buffer[n] = '\0';
	send(fd,buffer,BUFF_SIZE, 0);
	printf("%s\n",buffer);

	/* waits for client to answer*/
	n = recv(fd, buffer,BUFF_SIZE,0);

	/* cleans buffer */
	buffer[n] = '\0';


	/* prints message */
	printf("%s\n", buffer);

	/* connection request*/	
	if(strstr(buffer, "OK") == NULL) 	return -1;


	/* setup completed */
	print_clients(clients_head);


	return 0;

}




