/* pieces definitions */

#define PACMAN 'p'
#define POWER_PACMAN 'P'
#define MONSTER 'M'
#define BRICK 'B'
#define EMPTY ' '
#define LEMON 'L'
#define CHERRY 'C'
#define BUFF_SIZE 100
#define PIECE_SIZE 90

/* server definitions*/
#define SERVER_PORT "60000"
#define SECONDS_TIMEOUT 5
#define USECONDS_TIMEOUT 0

/* game definitions*/

#define FRUIT_ST_USECONDS 2000000
#define FRUITS_PER_PLAYER 2

#define POWER_PACMAN_COUNTER 2

#define SCORE_TIME_SECONDS 60

#define USER_MAX_TIME_USECONDS 500000
#define USER_TIMEOUT_USECONDS 30000000

/* files definitions*/
#define BOARD_FILE "board.txt"
#define FIFO_FILE  "pac_fifo"
#define FRUIT_FIFO_FILE  "fruit_fifo"
