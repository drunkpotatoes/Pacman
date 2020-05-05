# Pacman

## PROTOCOL

### Client-Server SETUP:

Client: RQ - Request Connection

If the server is full it sends a welcome message, otherwise, sends a wait message. Client will wait 10 seconds and re-attempt connection.

Server: WE - Welcome

Server: W8 - Wait

After receiving the welcome from the server, the client will send his colour information for the pacman and monster. The pacman RGB code will come first and the monster right after.

Client: CC [R,G,B] [R,G,B]

Server will them send a message mapping the board containing the number of pieces the clients needs to add to his board.

Server: MP  [ROW]:[COL] [NUMBER_PIECES] 

Server: PT  [PIECE]@[Y]:[X] [R,G,B]           

Server repeates PT until it lets the client know all pieces (NUMBER_PIECES) that need to be placed.
If the client doesn't receive all the pieces promised in the MP command it disconnects, otherwise sends okay message.

Client: OK - Okay

Client: DC - Disconnect


### Client-Server LOOP:

The game loop consists of the server telling the clients eitheir to clear or to put pieces.

Server: PT [PIECE]@[Y]:[X] [R,G,B]   

Server: CL [Y]:[X]

At any time the server or the client can disconnet sending the respective message.

Server: DC

Client: DC
