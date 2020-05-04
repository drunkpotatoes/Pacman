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


#include "client.h"

int main(int argc, char** argv)
{

	int fd,n;
	struct addrinfo **res;
	char buffer[100];

    struct timeval tv;


     /* sets time struct to 1 second */
    tv.tv_sec =1;
    tv.tv_usec = 0;

	res = malloc(sizeof(struct addrinfo*));

	while (1)
	{
		if ( (fd = client_connect(res, SERVER_IP, SERVER_PORT)) == -1)
		{
			return -1;
		}

		n = sprintf(buffer, "RQ\n");

		buffer[n] = '\0';

		/* sends a connection request to server */
		send(fd, buffer,BUFF_SIZE,0);

		/* sets a timeout of 1 second*/
		setsockopt(fd,SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

		n = recv(fd,buffer,BUFF_SIZE,0);

		buffer[n] = '\0';

		if(strstr(buffer,"WE") != NULL)
		{	

			printf("%s\n", buffer);
			return 0;


		}

		else if (strstr(buffer,"W8") != NULL)
		{
			printf("%s\n", buffer);
			/* closes connection */
			close(fd);
			sleep(10);
		}
		
		sleep(1);

	}
	

	getchar();

	return 0;
}