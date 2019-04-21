/**********************************************************/
/* This program is a UDP version of tictactoe      */
/* Two users, player 1 and player 2, pass the game back   */
/* and forth, on two computers                	          */
/**********************************************************/

/*
   The client is always player 1
   The server is always player 2
*/

/* include files go here */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include "tictactoe.h"


int main(int argc, char *argv[])
{


	/*Checking if user's inputs are valid.*/
	if(argc == 3)
	{
		if(atoi(argv[2]) == SERVER_NUMBER)
		{
			char *port = argv[1];
			server(port);
		} else 
		{
			printf("The player number for a server is 1.\n");
		}
	} else 
	{
		printf("Wrong number of arguments.\n");
		printf("./tictactoeServer <port> <player number> for a server.\n");
	}
  
	return 0; 
}

/*A function that initializes and binds a socket.*/
int server(char *port)
{

	int sd;             /*socket descriptor*/
	int current_number_of_clients = 0;
	//char board[ROWS][COLUMNS]; /*Creating a board.*/
	struct sockaddr_in server_addr;
	Client *list = NULL;
	Game_Number *list_numbers = NULL;

	printf("The port number: %s\n",port);

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(port));
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
	sd = socket(AF_INET,SOCK_DGRAM,0);

	/*if socket failed to initiate, report the error*/
	if(sd == -1)
	{
		printf("Failed to create a socket.\n");
		return -1;
	}

	/*Set other attributes to 0*/
	bzero(&(server_addr.sin_zero),8);

	/*bind the socket with ip address and port*/
	if(bind(sd,(struct sockaddr*)&server_addr,sizeof(struct sockaddr))<0)
	{
        	printf("ERROR: Bind error\n");
		close(sd);
        	return -1;
   	}

	initialize_game_numbers(&list_numbers, NUMBER_OF_CLIENTS);

	/*Keep listen the incoming client*/
	while(1)
  	{
		read_tictactoe(&list, &list_numbers, &current_number_of_clients, sd);
	}
	
	deallocate_game_number(&list_numbers, NUMBER_OF_CLIENTS);
	close(sd);    
	return 0;
}

int tictactoe(Client *current, int sd, uint8_t choice, uint8_t *buffer)
{
	uint8_t state_check;	//second byte
	uint8_t modifier = 1;	//Forth byte
	int row, column;
	//socklen_t address_length = sizeof(to_server)
	char mark = 'X'; 

	row = (int)((choice-1) / ROWS); 
	column = (choice-1) % COLUMNS;

	/*Check if the client choice is valid*/
	if (current->board[row][column] == (choice+'0'))
		{
			current->board[row][column] = mark;
			/*Check if client win*/
			if(checkwin(current->board) != -1)
			{
				state_check = 1;
				if(checkwin(current->board) == 1)
				{
					/* Client win */
					modifier = 3;
				} else 
				{
					/* Draw */
					modifier = 1;
				}
				set_clock(current);
				encode(&buffer, VERSION, NONE, GAME_COMPLETE, modifier, MOVE, current->game->number);
				sendto(sd, buffer, DATAGRAM_SIZE, NONE,(struct sockaddr*)&(current->client), sizeof(current->client));
				return state_check;
			}

		}else
		{
			/*Invalid move for client*/
			encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, NONE, current->game->number);
			sendto(sd, buffer, DATAGRAM_SIZE, NONE,(struct sockaddr*)&(current->client), sizeof(current->client));
			return -1;
    }
		
		/*Server do the choice*/
		choice = next_move(current->board);
		mark = 'O'; 

		row = (int)((choice-1) / ROWS); 
		column = (choice-1) % COLUMNS;
		current->board[row][column] = mark;
		/*Check if client win*/
		if(checkwin(current->board) != -1)
		{
			state_check = 1;
			if(checkwin(current->board) == 1)
			{
				/* Server win */
				modifier = 2;
			} else 
			{
				/* Draw */
				modifier = 1;
			}
			set_clock(current);
			encode(&buffer, VERSION, NONE, GAME_COMPLETE, modifier, MOVE, current->game->number);
			sendto(sd, buffer, DATAGRAM_SIZE, NONE,(struct sockaddr*)&(current->client), sizeof(current->client));
			return state_check;
		} else 
		{
			state_check = 0;
			set_clock(current);
			encode(&buffer, VERSION, choice, state_check, NONE, MOVE, current->game->number);
			sendto(sd, buffer, DATAGRAM_SIZE, NONE,(struct sockaddr*)&(current->client), sizeof(current->client));
		}

	return 0;
}


int checkwin(char board[ROWS][COLUMNS])
{
  /************************************************************************/
  /* brute force check to see if someone won, or if there is a draw       */
  /* return a 0 if the game is 'over' and return -1 if game should go on  */
  /************************************************************************/
  if (board[0][0] == board[0][1] && board[0][1] == board[0][2] ) // row matches
    return 1;
        
  else if (board[1][0] == board[1][1] && board[1][1] == board[1][2] ) // row matches
    return 1;
        
  else if (board[2][0] == board[2][1] && board[2][1] == board[2][2] ) // row matches
    return 1;
        
  else if (board[0][0] == board[1][0] && board[1][0] == board[2][0] ) // column
    return 1;
        
  else if (board[0][1] == board[1][1] && board[1][1] == board[2][1] ) // column
    return 1;
        
  else if (board[0][2] == board[1][2] && board[1][2] == board[2][2] ) // column
    return 1;
        
  else if (board[0][0] == board[1][1] && board[1][1] == board[2][2] ) // diagonal
    return 1;
        
  else if (board[2][0] == board[1][1] && board[1][1] == board[0][2] ) // diagonal
    return 1;
        
  else if (board[0][0] != '1' && board[0][1] != '2' && board[0][2] != '3' &&
	   board[1][0] != '4' && board[1][1] != '5' && board[1][2] != '6' && 
	   board[2][0] != '7' && board[2][1] != '8' && board[2][2] != '9')

    return 0; // Return of 0 means game over
  else
    return  - 1; // return of -1 means keep playing
}


void print_board(char board[ROWS][COLUMNS])
{
  /*****************************************************************/
  /* brute force print out the board and all the squares/values    */
  /*****************************************************************/

  printf("\n\n\n\tCurrent TicTacToe Game\n\n");

  printf("Player 1 (X)  -  Player 2 (O)\n\n\n");


  printf("     |     |     \n");
  printf("  %c  |  %c  |  %c \n", board[0][0], board[0][1], board[0][2]);

  printf("_____|_____|_____\n");
  printf("     |     |     \n");

  printf("  %c  |  %c  |  %c \n", board[1][0], board[1][1], board[1][2]);

  printf("_____|_____|_____\n");
  printf("     |     |     \n");

  printf("  %c  |  %c  |  %c \n", board[2][0], board[2][1], board[2][2]);

  printf("     |     |     \n\n");
}

int read_tictactoe(Client **list, Game_Number **list_numbers, int * current_number_of_clients, int sd)
{

	

	print_clients(*list, *current_number_of_clients);

	uint8_t *buffer = NULL;
	int recv_length = 0;
	
	socklen_t address_length;
	Client *current = NULL;
	struct sockaddr_in client;

	buffer = (uint8_t*) malloc(BUFFER_SIZE * sizeof(uint8_t));
	address_length = sizeof(client);
	memset(&client, 0, address_length);

	time_out_check(list, list_numbers, current_number_of_clients, buffer, sd);
	printf("Waiting for the data.\n");
	recv_length = recvfrom(sd, buffer, DATAGRAM_SIZE, 0, (struct sockaddr*) &client, &address_length);
	printf("Reveive: %u %u %u %u %u %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);

	printf("Just received the data.\n");
	if(recv_length < 0 )
	{
		printf("Faild to receive a data.\n");
		free(buffer);
		return 0;
	} else if (recv_length == 0) 
	{
		printf("Conenction Lost.\n");
		free(buffer);
		return 0;
	} 
	time_out_check(list, list_numbers, current_number_of_clients, buffer, sd);

	
	current = find_client(list, client, *current_number_of_clients);

	if (buffer[0] != VERSION)
	{
		printf("Just received a message of a wrong version number.\n");
		set_clock(current);
		encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, NONE, NONE);
		sendto(sd, buffer, DATAGRAM_SIZE, NONE,(struct sockaddr*)&(client), sizeof(client));
		time_out_check(list, list_numbers, current_number_of_clients, buffer, sd);
		return 0;
	}

	if(buffer[4] == NEW_GAME)
	{
		if(current != NULL)
		{
			delete_client(list, current, list_numbers, current_number_of_clients);
			set_clock(current);
			encode(&buffer, VERSION, NONE, GENERAL_ERROR, TRY_AGAIN, NONE, current->game->number);
			sendto(sd, buffer, DATAGRAM_SIZE, NONE,(struct sockaddr*)&(current->client), sizeof(current->client));
		} else
		{
			if(*current_number_of_clients < NUMBER_OF_CLIENTS)
			{
				current = insert_client(list, client, list_numbers, current_number_of_clients);
				set_clock(current);
				encode(&buffer, VERSION, NONE, NONE, NONE, NONE, current->game->number);
				sendto(sd, buffer, DATAGRAM_SIZE, NONE,(struct sockaddr*)&(current->client), sizeof(current->client));
				print_clients(*list, *current_number_of_clients);
			} else 
			{	
				encode(&buffer, VERSION, NONE, GENERAL_ERROR, OUT_OF_RESOURCES, NONE, NONE);
				sendto(sd, buffer, DATAGRAM_SIZE, NONE,(struct sockaddr*)&(client), sizeof(client));
			}
		}

	} else if(buffer[4] == MOVE)
	{
		if(current != NULL)
		{
			if(buffer[5] == current->game->number)
			{					
				tictactoe(current, sd, buffer[1], buffer);
			} else
			{
				if(buffer[5] >= 0 && buffer[5] < NUMBER_OF_CLIENTS)
				{
					printf("WARNING\nPlayer at a game number of %d is trying to access a game number %d.\n", current->game->number, buffer[5]);
				} else 
				{
					printf("Player at a game number of %d is trying to access an invalid game number %d.\n", current->game->number, buffer[5]);
				}
				set_clock(current);
				encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, NONE, NONE);
				sendto(sd, buffer, DATAGRAM_SIZE, NONE,(struct sockaddr*)&(current->client), sizeof(current->client));
			}
		} else
		{
			printf("WARNING\nSomeone is faking his/her identity.\nNone-client requested for a move.\n");
			encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, NONE, NONE);
			sendto(sd, buffer, DATAGRAM_SIZE, NONE,(struct sockaddr*)&(client), sizeof(client));
		}

	} else 
	{
		printf("Just received a message of a wrong command:%d.\n", buffer[4]);
		encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, MOVE, NONE);
		sendto(sd, buffer, DATAGRAM_SIZE, NONE, (struct sockaddr*)&(client), sizeof(client));
	}
	
	time_out_check(list, list_numbers, current_number_of_clients, buffer, sd);
	free(buffer);

	return 0;
}


int initSharedState(char board[ROWS][COLUMNS])
{    
    /* this just initializing the shared state aka the board */
    int i, j, count = 1;
    for (i=0;i<3;i++)
        for (j=0;j<3;j++){
            board[i][j] = count + '0';
            count++;
        }
    return 0;
}

int next_move(char board[ROWS][COLUMNS])
{
	int result = 0;
	int i,j;
	for(i = 0; i < 3; i++)
	{
		for(j = 0; j < 3; j++)
		{
			if(board[i][j] != 'X' && board[i][j] != 'O')
			{
				return 	i * 3 + (j+1);
			}
		}
	}
	return result;
}

void time_out_check(Client **list, Game_Number **list_numbers, int *current_number_of_clients, uint8_t *buffer, int sd)
{	
	time_t current_time = time(NULL);
	Client *current;
	int index;
	if((*current_number_of_clients) > 0)
	{
		printf("Checking for time outs...\n");
		current = *list;
		for(index = 0; index < *current_number_of_clients; index++)
		{
			if((current_time - current->time_out) >= TIME_OUT_INV)
			{
				printf("A client at a game number of %d has been removed due to its time out.\n", current->game->number);
				encode(&buffer, VERSION, NONE, GENERAL_ERROR, TIME_OUT, MOVE, current->game->number);
				sendto(sd, buffer, DATAGRAM_SIZE, NONE,(struct sockaddr*)&(current->client), sizeof(current->client));
				delete_client(list, current, list_numbers, current_number_of_clients);
			}
			current = current->next;			
		}
	}
}



void delete_client(Client **list, Client *client, Game_Number **head, int *current_number_of_clients)
{
	if(client->prev == NULL)
	{
		*list = client->next;
		if(client->next != NULL)
		{
			client->next->prev = *list;
		}
	} else 
	{
		client->prev->next = client->next;
		if(client->next != NULL)
		{
			client->next->prev = client->prev;
		}
	}

	return_game_number(head, client->game);
	free(client);
	*current_number_of_clients -= 1;
}

Client * insert_client(Client **list, struct sockaddr_in client, Game_Number **head, int *current_number_of_clients)
{
	Client *target = NULL;
	target = malloc(sizeof(Client));

	if(target == NULL)
	{
		printf("Failed to allocate a memory space for a client.\n");
		exit(0);
	}

	target->next = NULL;
	target->prev = NULL;
	target->client = client;
	target->player_number = 2;

	initSharedState(target->board);

	target->game = get_game_number(head);
	*current_number_of_clients += 1;

	if(*list != NULL)
	{
		target->next = *list;
		(*list)->prev = target;
	}
	
	*list = target;
	return target;

}

Client * find_client(Client **list, struct sockaddr_in client, int current_number_of_clients)
{
	Client *current = NULL;
	current = *list;
	int index;
	for(index = 0; index < current_number_of_clients; index++)
	{
		
		if(current->client.sin_addr.s_addr == client.sin_addr.s_addr)
		{
			return current;
		}
		current = current->next;
	}
	return NULL;
}

void initialize_game_numbers(Game_Number **head, unsigned int number_of_clients)
{
	Game_Number *current = NULL;
	uint8_t i;
	for(i = 0; i < number_of_clients; i++)
	{
		Game_Number *game = malloc(sizeof(Game_Number));
		game->number = i;
		game->next = NULL;
		
		if (i == 0)
		{
			*head = game;
			current = *head;
		} else 
		{
			current->next = game;
			current = current->next;
		}		
	}
}

void deallocate_game_number(Game_Number **head, unsigned int number_of_clients)
{
	Game_Number *target;
	uint8_t i = 0;
	for(i = 0; i < number_of_clients; i++)
	{
		target = *head;
		*head = (*head)->next;
		free(target);
	}
}

Game_Number * get_game_number(Game_Number **head)
{
	Game_Number *target = NULL;
	target = *head;
	*head = (*head)->next;
	target->next = NULL;
	return target;
}

void return_game_number(Game_Number **head, Game_Number *target)
{
	target->next = *head;
	*head = target;
}

void encode(uint8_t **buffer, uint8_t version, uint8_t position, uint8_t error, uint8_t modifier, uint8_t command, uint8_t game_number)
{
	memset(*buffer, 0, DATAGRAM_SIZE);
	memset(*buffer, version, 1);
	memset((*buffer)+1, position, 1);
	memset((*buffer)+2, error, 1);
	memset((*buffer)+3, modifier, 1);
	memset((*buffer)+4, command, 1);
	memset((*buffer)+5, game_number, 1);	
}

void set_clock(Client *current)
{
	if(current != NULL)
	{
		current->time_out = time(NULL);
	}
}


void print_clients(Client *list, int current_number_of_clients)
{
	Client *current = list;
	int i;
	printf("Current player list:\n");
	for(i = 0; i < current_number_of_clients;i++)
	{
		printf("Game number: %u.\n", current->game->number);
		current = current->next;
	}
	printf("\n");
}
