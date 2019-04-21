/* #define section, for now we will define the number of rows and columns */

#define BLANK_SPACE 0
#define BOARD 7
#define BUFFER_SIZE 1000
#define COLUMNS  3
#define CLIENT_NUMBER 2
#define CLIENT_SPACE 1
#define CLIENT_WINS 2
#define DATAGRAM_SIZE 7
#define DRAW 1
#define END_GAME 2
#define FAIL 0
#define GENERAL_ERROR 2
#define GAME_COMPLETE 1
#define GAME_IN_PROGRESS 0
#define INVALID_REQUEST 2
#define MAX_SEQUENCE 255
#define MC_GROUP "239.0.0.1"
#define MC_PORT 1818
#define MIN_SEQUENCE 0
#define MOVE 1
#define NUMBER_OF_CLIENTS 3
#define NEW_GAME 0
#define NONE 0
#define OUT_OF_RESOURCES 1
#define ROWS  3
#define RECONNECT 3
#define SERVER_NUMBER 1
#define SERVER_SPACE 2
#define SERVER_WIN 3
#define SEQUENCE 6
#define SHUT_DOWN 3
#define SUCCESS 1
//define TIME_OUT 5
//#define TIME_OUT_INV 10
#define TRY_AGAIN 5
#define VERSION 8

typedef struct Game_Number{
	uint8_t number;
	struct Game_Number *next;
} Game_Number;

typedef struct Client {

	int sequence_number;
	int player_number;
	Game_Number *game;
	char board[ROWS][COLUMNS];
	struct Client *next;
	struct Client *prev;
	struct sockaddr_in client;

} Client;



/* C language requires that you predefine all the routines you are writing */
int checkwin(char board[ROWS][COLUMNS]);
int client(char *ip_addr, char *port);
void encode(uint8_t **buffer, uint8_t version, uint8_t position, uint8_t error, uint8_t modifier, uint8_t command, uint8_t game_number, uint8_t sequence);
Client * find_client(Client **list, struct sockaddr_in client, int current_number_of_clients);
void reconnect_encode(uint8_t **buffer, uint8_t version, uint8_t position, uint8_t error, uint8_t modifier, uint8_t command, uint8_t game_number, uint8_t sequence, char board[ROWS][COLUMNS]);
void deallocate_game_number(Game_Number **head, unsigned int number_of_clients);
void delete_client(Client **list, Client *client, Game_Number **head, int *current_number_of_clients);
Game_Number * get_game_number(Game_Number **head);
void initialize_game_numbers(Game_Number **head, unsigned int number_of_clients);
int initSharedState(char board[ROWS][COLUMNS]);
Client * insert_client(Client **list, struct sockaddr_in client, Game_Number **head, int *current_number_of_clients, uint8_t *buffer, int command);
int next_move(char board[ROWS][COLUMNS]);
void print_board(char board[ROWS][COLUMNS]);
void print_clients(Client *list, int current_number_of_clients);
int read_tictactoe(Client **list, Game_Number **list_numbers, int * current_number_of_clients, int sd, uint8_t *buffer, struct sockaddr_in client);
void return_game_number(Game_Number **head, Game_Number *target);
int server(char *port);
int server_move(Client *current, int sd, uint8_t *buffer, int reconnect);
int seq_check(Client *current, uint8_t *buffer);
int tictactoe(Client *current, int sd, uint8_t choice, uint8_t *buffer);
int tictactoe_client(char board[ROWS][COLUMNS], int sd, struct sockaddr_in to_server,uint8_t *recv_buffer,uint8_t *send_buffer,uint8_t *sequence, int player);
int update_seq(Client *current, uint8_t *buffer);
struct sockaddr_in * client_multicast(uint8_t *recv_buffer,uint8_t *send_buffer, struct sockaddr_in *recv, int * number_of_servers);




























