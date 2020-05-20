#include "defs.h"
#include "utils.h"
#include "sockets.h"
#include "UI_library.h"

typedef struct Event_ShowSomething_Data{
	int x;
	int y;
	int piece;
	int r;
	int g;
	int b;
} Event_ShowSomething_Data;

void * server_listen_thread(void*);
int server_setup(int*, char*,char*);
void server_disconnect(int);
int game_loop(int fd);
