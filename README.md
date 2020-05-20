# Pacman

## CLIENT-SERVER PROTOCOLS

### SETUP:

Client Request a Connection.

Client: RQ 

If the server is free it sends a welcome message, otherwise, sends a wait message. Client will wait 10 seconds and re-attempt connection.

Server: WE # USER_ID

Server: W8 

After receiving the welcome from the server, the client will send his colour information for the pacman and monster. The pacman RGB code will come first and the monster right after.

Client: CC [R,G,B] [R,G,B]

Server will them send a message mapping the board containing the number of rows and columns of the board.

Server: MP  ROW:COL 

Server: PT  PIECE @ Y:X [R,G,B] # USER_ID           

Server repeates PT until it has sent all the non-empty pieces. After sending all the pieces, it will send a summary message containing the number of pieces sent.
The client will see this message and confirm if it received all of  the pieces or not, if not, disconnects.

Server: SS NUM_PIECES

Client: OK 

Client: DC 


### GAME LOOP:

The game loop consists of the server telling the clients eitheir to clear or to put pieces. And for the client to ask the server to move pieces.

Server: PT PIECE @ Y:X [R,G,B] # USER_ID

Server: CL Y:X

Client: MV PIECE @ PREV-Y:PREV-X => Y:X


At any time the server or the client can disconnet sending the respective message.

Server: DC

Client: DC


## THREAD COMUNICATION

### FIFOS


All client threads will comunicate with the main thread through a FIFO in order to report plays. The message sent to the main thread is the message it will be sent to the client. The main thread doesn't check anything about the message, it just fowards it to all clients.

Client Thread: PT PIECE @ Y:X [R,G,B] # USER_ID

All clients  threads will also comunicate with the fruit thread in order to ask for a fruit spawn. They will send a message in the format bellow containing the time when the message was sent.

Client Thread: TM SECONDS USECONDS

The fruit thread will read this message and compute the diference between the time when reading to the time in the message (lets call the result: delta). After that it will check if delta is equal or greater than the spawning time. If it is it will print the fruit right away. Otherwise it will sleep for the remaning time and print the fruit after it wakes up. This method was used to make the fruit thread as light on the processor as possible.

## GLOBAL VARIABLES

typedef struct board_infos{

    board_piece** board;
    int   row;
    int   col;
    int   empty;
    int   max_fruits;
    int   cur_fruits;

}board_info

typedef struct _client{

    unsigned long user_id;
    int fid;
   
    int p_r;
    int p_g;
    int p_b;
   
    int m_r;
    int m_g;
    int m_b;
   
    int fruit_score;
    int player_score;
   
    struct _client *next;

}client;

