/* #define section, for now we will define the number of rows and columns */

#define BUFFER_SIZE 1000
#define COLUMNS  3
#define CLIENT_NUMBER 2
#define DATAGRAM_SIZE 6
#define GENERAL_ERROR 2
#define GAME_COMPLETE 1
#define GAME_IN_PROGRESS 0
#define INVALID_REQUEST 2
#define MOVE 1
#define NUMBER_OF_CLIENTS 2
#define NEW_GAME 0
#define NONE 0
#define OUT_OF_RESOURCES 1
#define ROWS  3
#define SERVER_NUMBER 1
#define SHUT_DOWN 3
#define TIME_OUT 4
#define TIME_OUT_INV 60
#define TRY_AGAIN 5
#define VERSION 5

typedef struct Game_Number{
	uint8_t number;
	struct Game_Number *next;
} Game_Number;

typedef struct Client {
	char board[ROWS][COLUMNS];
	time_t time_out;
	int player_number;
	Game_Number *game;
	struct Client *next;
	struct Client *prev;
	struct sockaddr_in client;
} Client;




/* C language requires that you predefine all the routines you are writing */
int checkwin(char board[ROWS][COLUMNS]);
int client(char *ip_addr, char *port);
void encode(uint8_t **buffer, uint8_t version, uint8_t position, uint8_t error, uint8_t modifier, uint8_t command, uint8_t game_number);
Client * find_client(Client **list, struct sockaddr_in client, int current_number_of_clients);
void deallocate_game_number(Game_Number **head, unsigned int number_of_clients);
void delete_client(Client **list, Client *client, Game_Number **head, int *current_number_of_clients);
Game_Number * get_game_number(Game_Number **head);
void initialize_game_numbers(Game_Number **head, unsigned int number_of_clients);
int initSharedState(char board[ROWS][COLUMNS]);
Client * insert_client(Client **list, struct sockaddr_in client, Game_Number **head, int *current_number_of_clients);
int next_move(char board[ROWS][COLUMNS]);
void print_board(char board[ROWS][COLUMNS]);
void print_clients(Client *list, int current_number_of_clients);
int read_tictactoe(Client **list, Game_Number **list_numbers, int * current_number_of_clients, int sd);
void return_game_number(Game_Number **head, Game_Number *target);
int server(char *port);
void set_clock(Client *current);
int tictactoe(Client *current, int sd, uint8_t choice, uint8_t *buffer);
int tictactoe_client(char board[ROWS][COLUMNS], int sd, struct sockaddr_in to_server,uint8_t *recv_buffer,uint8_t *send_buffer);
void time_out_check(Client **list, Game_Number **list_numbers, int *current_number_of_clients,uint8_t *buffer, int sd);

























