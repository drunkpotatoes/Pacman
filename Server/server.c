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


/* decrements by 4 for each user - 2 for pacman and monters, 2 for the extra fruits */
board_info status;

void * client_thread (void* arg)
{
	
	int  fd;
	fd = *((int*) arg);

	if (client_setup(fd) == -1) 		return NULL;


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

	pthread_create(&thread_id , NULL, accept_thread, NULL);

	getchar();


	return 0;
}



int client_setup(int fd)
{	

	int n, num_pieces;
	
	char buffer[BUFF_SIZE];

	/*****************************************/
		/******* needs read protected zone******/
		/***************************************/
	if (status.empty >= 4)
	{
		n = sprintf(buffer,"WE\n");

		buffer[n] = '\0';

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

		send(fd,buffer,BUFF_SIZE,0);

		/* closes connection*/
		close(fd);

		return -1;
	}



	/* non empty pieces */


	num_pieces = status.row*status.col - status.empty;

	n = sprintf(buffer, "MP %d:%d %d\n", status.row, status.col, num_pieces);
	buffer[n] = '\0';
	send(fd,buffer,BUFF_SIZE,0);

	/* TO DO : send all pieces ;; NEED: place to save other user's colours and how to distinguish users
	n = sprintf(buffer, "PT %d:%d %d")
	for (i = 0; i < num_pieces; i++)
	{

	}
	*/
	return 0;

}




