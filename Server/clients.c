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

#include "clients.h"


void add_client(client** client_list, unsigned long id, int fd, int* pac, int *mon)
{
	client* new_cli;

	/* first client */
	if (*client_list == NULL)
	{	

		if ((new_cli = (client *) malloc(sizeof(client)) ) == NULL) 	mem_err("Client Struct");

		new_cli->user_id = id;
		new_cli->fid = fd;
		new_cli->p_r = pac[0];
		new_cli->p_g = pac[1];
		new_cli->p_b = pac[2];

		new_cli->m_r = mon[0];
		new_cli->m_g = mon[1];
		new_cli->m_b = mon[2];

		new_cli->next = NULL;

		*client_list = new_cli;

		return;
	}

	else
	{

		/* finds last position*/
		client* aux = *client_list;

		while(aux->next != NULL)
		{
			aux = aux->next;
		}


		/* allocates position for new client*/
		if ( ( new_cli = (client*) malloc(sizeof(client)) ) == NULL) 	mem_err("Client Struct");

		new_cli->user_id = id;
		new_cli->fid = fd;
		new_cli->p_r = pac[0];
		new_cli->p_g = pac[1];
		new_cli->p_b = pac[2];

		new_cli->m_r = mon[0];
		new_cli->m_g = mon[1];
		new_cli->m_b = mon[2];

		new_cli->next = NULL;
		new_cli->next = NULL;

		/* links previous client to this*/
		aux->next = new_cli;



		return;
	}
}



void print_clients(client * head)
{
	client* aux;

	if(head == NULL)
	{
		printf("\nNo clients.\n");
		return;
	}

	aux = head;

    while (aux != NULL) 
    {
        printf("\n| User_id: %lx | RGB PAC %d,%d,%d  | RGB MON %d,%d,%d |\n", 
        aux->user_id, aux->p_r, aux->p_g, aux->p_b, aux->p_r, aux->p_g,aux->p_b);
        
        aux = aux->next;
    }
}


client * search_client (client* head, unsigned long id)
{
	if(head == NULL)
	{
		return NULL;
	}
	client* aux;

	aux = head;
  

    while (aux != NULL) 
    {

        if (aux->user_id == id)
        {
        	
        	return aux;
        }
        
        aux = aux->next;
    }

    return NULL;
}


void remove_client(client ** head, unsigned long id)
{

	client* aux;
	client* prev;

	aux = *head;                    
 

  	if (aux != NULL && aux->user_id == id)
  	{
  		*head = aux->next;
  		free(aux);
  		return;
  	}

    while (aux != NULL && aux->user_id != id) 
    {

        prev = aux;
        aux = aux->next;
       
    }

    /* not found */
    if(aux == NULL) 	return;

    prev->next = aux->next;

    free(aux);

    return;
}


void free_clients(client* head)

{
	client* aux;


	while(head != NULL)
	{
		aux = head;
		head = head->next;
		free(aux);
	}

	return;
}


void get_pac_rgb(client* head, unsigned long id, int* r, int* g, int*b)
{

	if(head == NULL)
	{
		return;
	}
	client* aux;

	aux = head;

    while (aux != NULL) 
    {
    	
        if (aux->user_id == id)
        {
        	
        	*r = aux->p_r;
        	*g = aux->p_g;
        	*b = aux->p_b;

        	return;
        }
        
        aux = aux->next;
    }

	return;
}

void get_mon_rgb(client* head, unsigned long id ,int* r, int* g, int* b)
{

	if(head == NULL)
	{
		return;
	}

	client* aux;

	aux = head;

    while (aux != NULL) 
    {
    
    	
        if (aux->user_id == id)
        {
        	
        	*r = aux->m_r;
        	*g = aux->m_g;
        	*b = aux->m_b;

        	return;
        }
        
        aux = aux->next;
    }

	return;
}


int send_all_clients(client* head, char* msg, int msg_size)
{
	client* aux;


	if(head == NULL)
	{
		return -1;
	}

	aux = head;

    while (aux != NULL) 
    {
    	/* sends message to all clients*/
        send(aux->fid, msg, msg_size,0);

		aux = aux->next;
    }

    return 0;
}


int number_of_clients(client* head)
{

	int i;
	client* aux;

	i = 0;

	if(head == NULL)
	{
		return 0;
	}

	aux = head;

    while (aux != NULL) 
    {
    	/* increments number of clients*/
        i++;
		aux = aux->next;
    }

    return i;
}