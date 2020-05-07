# Pacman

## PROTOCOL

### Client-Server SETUP:

Client Request a Connection.

Client: RQ 

If the server is free it sends a welcome message, otherwise, sends a wait message. Client will wait 10 seconds and re-attempt connection.

Server: WE 

Server: W8 

After receiving the welcome from the server, the client will send his colour information for the pacman and monster. The pacman RGB code will come first and the monster right after.

Client: CC [R,G,B] [R,G,B]

Server will them send a message mapping the board containing the number of rows and columns of the board.

Server: MP  ROW:COL 

Server: PT  PIECE @ Y:X [R,G,B]           

Server repeates PT until it has sent all the non-empty pieces. After sending all the pieces, it will send a summary message containing the number of pieces sent.
The client will see this message and confirm if it received all of  the pieces or not, if not, disconnects.

Server: SS NUM_PIECES

Client: OK 

Client: DC 


### Client-Server LOOP:

The game loop consists of the server telling the clients eitheir to clear or to put pieces. And for the client to ask the server to move pieces.

Server: PT PIECE @ Y:X [R,G,B]   

Server: CL Y:X

Client: MV PIECE @ PREV-Y:PREV-X => Y:X

At any time the server or the client can disconnet sending the respective message.

Server: DC

Client: DC
