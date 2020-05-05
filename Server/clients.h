#include "defs.h"
#include "utils.h"


typedef struct _client
{
   unsigned long user_id;

   int p_r;
   int p_g;
   int p_b;

   int m_r;
   int m_g;
   int m_b;


   struct _client *next;

}client;

void add_client(client**,unsigned long, int*, int*);
void print_clients(client *);
