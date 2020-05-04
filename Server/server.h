#include "defs.h"
#include "board.h"
#include "utils.h"
#include "sockets.h"

void * client_thread(void*);
void * accept_thread (void*);
int    client_setup(int);
