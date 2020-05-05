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



void add_client(client** client_list, unsigned long id, int* pac, int *mon)
{
	client* new_cli;

	/* first client */
	if (*client_list == NULL)
	{	

		new_cli = (client *) malloc(sizeof(client));

		new_cli->user_id = id;
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
		new_cli = (client*) malloc(sizeof(client));

		new_cli->user_id = id;
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

	if(head == NULL)
	{
		printf("WTF\n");
	}
	client* aux;

	aux = head;
  

    while (aux != NULL) 
    {
        printf("\n| User_id: %lx | RGB PAC %d,%d,%d  | RGB MON %d,%d,%d |\n", 
        aux->user_id, aux->p_r, aux->p_g, aux->p_b, aux->p_r, aux->p_g,aux->p_b);
        
        aux = aux->next;
    }
}
