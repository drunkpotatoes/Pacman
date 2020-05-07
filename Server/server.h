#include "defs.h"
#include "board.h"
#include "utils.h"
#include "sockets.h"
#include "clients.h"

void * client_thread(void*);
void * accept_thread (void*);
int    client_setup(int);

void write_play_to_main();
