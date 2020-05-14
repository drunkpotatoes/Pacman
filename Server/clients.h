#include "defs.h"
#include "utils.h"


typedef struct _client
{
   unsigned long user_id;

   int fid;

   int p_r;
   int p_g;
   int p_b;

   int m_r;
   int m_g;
   int m_b;


   struct _client *next;

}client;

void add_client(client**,unsigned long,int,int*, int*);
void print_clients(client *);
client* search_client(client*, unsigned long);
void free_clients(client*);
void remove_client(client**, unsigned long);
void get_pac_rgb(client*, unsigned long,int*, int*, int*);
void get_mon_rgb(client*, unsigned long, int*, int*, int*);
int send_all_clients(client*, char*, int);
