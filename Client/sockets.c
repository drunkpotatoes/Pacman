
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "sockets.h"

/******************************************************************************
 * client_connect()
 *
 * Arguments:
 *			 struct addrinfo** - pointer to the struct that stores the
 *			 connection information
 *			 char* - IP of the server
 *			 char* - Port of the server
 * Returns:
 *			 fd - socket file ID
 * Side-Effects:
 *
 * Description: Establishes a connection with a TCP server.
 *
 ******************************************************************************/

int client_connect(struct addrinfo **res, char *ip, char* port)
{
    int fd,n;
    struct addrinfo hints;
    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_flags=AI_NUMERICHOST|AI_NUMERICSERV;
    


    /*gets server address info*/
    n = getaddrinfo(ip,port,&hints,res);
    if(n!=0)
    {
        fprintf(stderr,"\nERROR: getaddrinfo: %s\n",gai_strerror(n));
        return -1;
    }

    /*opens a socket*/
    fd=socket((*res)->ai_family,(*res)->ai_socktype,(*res)->ai_protocol);
    if(fd==-1)
    {
        fprintf(stderr,"\nERROR: socket\n");
        return -1;
    }

    /*establishes a connection with the socket*/
    n=connect(fd,(*res)->ai_addr,(*res)->ai_addrlen);
    if(n==-1)
    {
        printf("\nERROR: connect\n");
        return -1;
    }

    return fd;
}

/******************************************************************************
 * server_open()
 *
 * Arguments:
 *			 struct addrinfo** - pointer to the struct that stores the
 *			 connection information
 *			 char* - Port of the server
 * Returns:
 *			 fd - socket file ID
 * Side-Effects:
 *
 * Description: Opens TCP server.
 *
 ******************************************************************************/

int server_open(struct addrinfo **res,char* port)
{
    struct addrinfo hints;
    int fd,n;

    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_flags=AI_PASSIVE|AI_NUMERICSERV;
    
    /*gets own address info*/
    n=getaddrinfo(NULL,port,&hints,res);
    if(n!=0)
    {
        fprintf(stderr,"ERROR: getaddrinfo: %s\n",gai_strerror(n));
        return -1;
    }

    /*opens a socket*/
    fd = socket((*res)->ai_family,(*res)->ai_socktype,(*res)->ai_protocol);
    if(fd==-1)
    {
        printf("ERROR: socket\n");
        return -1;
    }

    /*assignes the socket to an address*/
    n = bind(fd,(*res)->ai_addr,(*res)->ai_addrlen);
    if(n==-1)
    {
        printf("ERROR: bind_TCP\n");
        return -1;
    }
    
    /*waits for connection*/
    n = listen(fd,5)==-1;
    if(n == -1)
    {
        printf("ERROR: listen\n");
        return -1;
    }

    return fd;
}

/******************************************************************************
 * close_network()
 *
 * Arguments:
 *			 struct addrinfo** - pointer to the struct that stores the
 *			 connection information
 *			 int - socket file ID
 * Returns:
 *			
 * Side-Effects:
 *
 * Description: Closes the connection.
 *
 *****************************************************************************/

void close_connection(struct addrinfo **res, int fd)
{
    /*fres address information*/
    freeaddrinfo(*res);
    res=NULL;
    /*closes socket*/
    close(fd);
    
    return;
}
