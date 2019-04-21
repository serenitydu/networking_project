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
	if(argc == 2)
	{
		char *port = argv[1];
		server(port);
	} else 
	{
		printf("Wrong number of arguments.\n");
		printf("./tictactoeServer <port> for a server.\n");
	}
  
	return 0; 
}

/*A function that initializes and binds a socket.*/
int server(char *port)
{

	int sd;             /*socket descriptor*/
	int maximum_sd = 0;
	int current_number_of_clients = 0;
	int i;
	socklen_t length;
	int sd_list[NUMBER_OF_CLIENTS];
	struct sockaddr_in client_list[NUMBER_OF_CLIENTS];

	memset(sd_list, 0, sizeof(int) * NUMBER_OF_CLIENTS);
	memset(client_list, 0, sizeof(struct sockaddr_in) * NUMBER_OF_CLIENTS);

	uint8_t *buffer = (uint8_t*) malloc(BUFFER_SIZE * sizeof(uint8_t));

	if(buffer == NULL)
	{
		printf("Failed to allocate a memory space.\n");
		return 0;
	}

	struct sockaddr_in server_addr;
	Client *list = NULL;
	Game_Number *list_numbers = NULL;
	fd_set fds;

	printf("The port number: %s\n",port);

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(port));
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
	/*create socket in TCP mode*/
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
		printf("Failed to create a socket.\n");
		exit(0); 
  	} 

	maximum_sd = sd;
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

	listen(sd, 1);
	/*Keep listen the incoming client*/
	while(1)
  	{
		int new_sd;
		struct sockaddr_in client;
		FD_ZERO(&fds);
		FD_SET(sd, &fds);

		for(i = 0; i < NUMBER_OF_CLIENTS; i++)
		{
			if(sd_list[i] > 0)
			{
				FD_SET(sd_list[i], &fds);
				if(sd_list[i] > maximum_sd)
					maximum_sd = sd_list[i];
			}
		}

		select(maximum_sd + 1, &fds, NULL, NULL, NULL);
			
		if(FD_ISSET(sd, &fds) && current_number_of_clients < NUMBER_OF_CLIENTS)
		{
			printf("Select reported.\n");
			new_sd = accept(sd, (struct sockaddr *) &client, &length);
			printf("Accept reported.\n");
			if(current_number_of_clients < NUMBER_OF_CLIENTS && (find_client(&list, client, current_number_of_clients) == NULL))
			{
				for(i = 0; i < NUMBER_OF_CLIENTS; i++)
				{
					if(sd_list[i] == 0)
					{
						printf("Added to sd list\n");
						sd_list[i] = new_sd;
						client_list[i] = client;
						break;
					}
				}
			}
		}

		for(i = 0; i < NUMBER_OF_CLIENTS; i++)
		{
			if(sd_list[i] != 0 && FD_ISSET(sd_list[i], &fds))
			{
				printf("Waiting for the data.\n");
				int recv_length = read(sd_list[i], buffer, BUFFER_SIZE);
				
				if(recv_length <= 0 )
				{
					printf("Conenction Lost.\n");
					close(sd_list[i]);
					sd_list[i] = 0;
					memset(client_list+i, 0, sizeof(struct sockaddr_in));
				} else 
				{
					printf("Just received the data.\n");
					printf("Receive: %u %u %u %u %u %u %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
					if(read_tictactoe(&list, &list_numbers, &current_number_of_clients, sd_list[i], buffer, client_list[i]) == -1)					{
						close(sd_list[i]);
						sd_list[i] = 0;
						memset(client_list+i, 0, sizeof(struct sockaddr_in));
					}
				}
			}
			memset(buffer, 0, sizeof(uint8_t) * BUFFER_SIZE);
		}	
		
	}
	
	deallocate_game_number(&list_numbers, NUMBER_OF_CLIENTS);
	free(buffer);
	close(sd);    
	return 0;
}

int tictactoe(Client *current, int sd, uint8_t choice, uint8_t *buffer)
{
	uint8_t state_check;	//second byte
	uint8_t modifier = 1;	//Forth byte
	int row, column;
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
				if(buffer[2] != GAME_COMPLETE)
				{
					printf("Game is completed but the client claims it did not.\n");
					encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, MOVE, current->game->number, current->sequence_number);
					write(sd,buffer,BUFFER_SIZE);
					printf("Sending: %u %u %u %u %u %u %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
					return -1;
				}

				state_check = 1;
				if(checkwin(current->board) == 1)
				{
					/* Client wins */
					modifier = CLIENT_WINS;	

					if(buffer[3] != CLIENT_WINS)
					{
						printf("The client claims the client did not win.\n");
						encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, END_GAME, current->game->number, current->sequence_number);
						write(sd,buffer,BUFFER_SIZE);
						printf("Sending: %u %u %u %u %u %u %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
						return -1;
					}


				} else if(checkwin(current->board) == 0)
				{
					/* Client draws*/
					modifier = DRAW;
					if(buffer[3] != DRAW)
					{
						printf("The client claims the game is not drawed.\n");
						encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, END_GAME, current->game->number, current->sequence_number);
						write(sd,buffer,BUFFER_SIZE);
						printf("Sending: %u %u %u %u %u %u %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
						return -1;
					}
	

				} 

				encode(&buffer, VERSION, NONE, GAME_COMPLETE, modifier, END_GAME, current->game->number, current->sequence_number);
				write(sd,buffer,BUFFER_SIZE);
				printf("Sending: %u %u %u %u %u %u %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
				update_seq(current, buffer);
				return -1;
			} else 
			{
				/* Game in progress */
				if(buffer[2] == GAME_COMPLETE)
				{
					printf("The game is still in progress but the client claims it is ended.\n");
					encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, MOVE, current->game->number, current->sequence_number);
					write(sd,buffer,BUFFER_SIZE);
					printf("Sending: %u %u %u %u %u %u %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
					update_seq(current, buffer);
					return -1;
				}
			}

		}else
		{
			printf("Invalid move for client\n");
			encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, NONE, current->game->number, current->sequence_number);
			write(sd,buffer,BUFFER_SIZE);
			printf("Sending: %u %u %u %u %u %u %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
			update_seq(current, buffer);
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
				modifier = SERVER_WIN;
			} else 
			{
				/* Draw */
				modifier = DRAW;
			}
			encode(&buffer, VERSION, choice, GAME_COMPLETE, modifier, MOVE, current->game->number, current->sequence_number);
			write(sd,buffer,BUFFER_SIZE);
			printf("Sending: %u %u %u %u %u %u %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
			update_seq(current, buffer);
			return state_check;
		} else 
		{
			state_check = 0;
			encode(&buffer, VERSION, choice, state_check, NONE, MOVE, current->game->number, current->sequence_number);
			write(sd,buffer,BUFFER_SIZE);
			printf("Sending: %u %u %u %u %u %u %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
			update_seq(current, buffer);
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

int read_tictactoe(Client **list, Game_Number **list_numbers, int *current_number_of_clients, int client_sd, uint8_t *original_buffer, struct sockaddr_in client_structure)
{
	print_clients(*list, *current_number_of_clients);

	uint8_t *buffer = original_buffer;
	int sequence = 0;
	
	Client *current = find_client(list, client_structure, *current_number_of_clients);
	

	if(current != NULL && (seq_check(current, buffer) == FAIL))
	{
		printf("Wrong sequence.\nReceived: %u %u %u %u %u %u %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
		write(client_sd, buffer, BUFFER_SIZE);
		printf("Wrong sequence.\nSending: %u %u %u %u %u %u %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
		return 0;
	
	}

	if(buffer[6] == MAX_SEQUENCE)
	{
		sequence = MIN_SEQUENCE;
	} else 
	{
		sequence = buffer[6] + 1;
	}
	

	if (buffer[0] != VERSION)
	{
		printf("Just received a message of a wrong version number.\n");
		
		if(current != NULL)
		{
			encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, NONE, NONE, current->sequence_number);
			write(client_sd,buffer,BUFFER_SIZE);
			printf("Wrong version\nclient\nSending: %u %u %u %u %u %u %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
			update_seq(current, buffer);
		} else 
		{

			encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, NONE, NONE, sequence);
			write(client_sd,buffer,BUFFER_SIZE);
			printf("Wrong version\nnot client\nSending: %u %u %u %u %u %u %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
		}


		return 0;
	}

	if(buffer[4] == NEW_GAME)
	{
		if(current != NULL)
		{
			
			encode(&buffer, VERSION, NONE, GENERAL_ERROR, TRY_AGAIN, NONE, current->game->number, current->sequence_number);
			write(client_sd,buffer,BUFFER_SIZE);
			printf("New game\nnot client\nSending: %u %u %u %u %u %u %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
			delete_client(list, current, list_numbers, current_number_of_clients);
			return -1;
		} else
		{
			if(*current_number_of_clients < NUMBER_OF_CLIENTS)
			{
				current = insert_client(list, client_structure, list_numbers, current_number_of_clients, buffer);
				if(current != NULL)
				{
					encode(&buffer, VERSION, NONE, NONE, NONE, NONE, current->game->number, current->sequence_number);
					write(client_sd,buffer,BUFFER_SIZE);
					printf("New game\nclient\nSending: %u %u %u %u %u %u %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
					update_seq(current, buffer);

					print_clients(*list, *current_number_of_clients);
				} else
				{
					printf("Error: failed to allocate a memory space for a client.\n");
				}
			} else 
			{	
				printf("Not enough space for a new client.\n");
				encode(&buffer, VERSION, NONE, GENERAL_ERROR, OUT_OF_RESOURCES, NONE, NONE, sequence);
				write(client_sd,buffer,BUFFER_SIZE);
				printf("Sending: %u %u %u %u %u %u %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
			}
		}

	} else if(buffer[4] == MOVE)
	{
		if(current != NULL)
		{
			if(buffer[5] == current->game->number)
			{					
				int status = tictactoe(current, client_sd, buffer[1], buffer);
				print_board(current->board);
				if(status == -1)
				{
					delete_client(list, current, list_numbers, current_number_of_clients);
					return -1;
				}
							
			} else
			{
				if(buffer[5] >= 0 && buffer[5] < NUMBER_OF_CLIENTS)
				{
					printf("WARNING\nPlayer at a game number of %d is trying to access a game number %d.\n", current->game->number, buffer[5]);
				} else 
				{
					printf("Player at a game number of %d is trying to access an invalid game number %d.\n", current->game->number, buffer[5]);
				}
				encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, NONE, NONE, current->sequence_number);
				write(client_sd,buffer,BUFFER_SIZE);
				printf("Sending: %u %u %u %u %u %u %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
				update_seq(current, buffer);
			}
		} else
		{
			printf("WARNING\nSomeone is faking his/her identity.\nNon-client requested for a move.\n");
		
			encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, NONE, NONE, sequence);
			write(client_sd,buffer,BUFFER_SIZE);
			printf("Sending: %u %u %u %u %u %u %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
		}

	} else if(buffer[4] == END_GAME)
	{
		if(current != NULL)
		{
			delete_client(list, current, list_numbers, current_number_of_clients);
			memset(buffer, 0, BUFFER_SIZE);
			return -1;
		} else 
		{
			printf("Not a client but wants to end the game.\n");
			encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, NONE, NONE, sequence);
			write(client_sd,buffer,BUFFER_SIZE);
			printf("End game\nnot client\nSending: %u %u %u %u %u %u %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
		}	
	} else 
	{

		printf("Just received a message of a wrong command:%d.\n", buffer[4]);
		

		if(current != NULL)
		{
			encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, MOVE, NONE, current->sequence_number);
			write(client_sd,buffer,BUFFER_SIZE);
			printf("Command invalid\nNot client\nSending: %u %u %u %u %u %u %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
			update_seq(current, buffer);
		} else 
		{
			encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, MOVE, NONE, sequence);
			write(client_sd,buffer,BUFFER_SIZE);
			printf("Command invalid\nClient\nSending: %u %u %u %u %u %u %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
		}
	}

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


/*
	deleting a client from the list
*/
void delete_client(Client **list, Client *client, Game_Number **head, int *current_number_of_clients)
{
	printf("Removing the client.\n");
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
	memset(&(client->client), 0, sizeof(struct sockaddr_in));
	free(client);
	*current_number_of_clients -= 1;
}

/*
	Inserting a client to the list.
*/
Client * insert_client(Client **list, struct sockaddr_in client, Game_Number **head, int *current_number_of_clients, uint8_t *buffer)
{
	printf("A client is inserted.\n");
	Client *target = NULL;
	target = malloc(sizeof(Client));

	if(target == NULL)
	{
		printf("Failed to allocate a memory space for a client.\n");
		exit(0);
	}

	target->sequence_number = buffer[6] + 1;
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

/*
	Finding a client from the list.
*/
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

void encode(uint8_t **buffer, uint8_t version, uint8_t position, uint8_t error, uint8_t modifier, uint8_t command, uint8_t game_number, uint8_t sequence)
{
	memset(*buffer, 0, DATAGRAM_SIZE);
	memset(*buffer, version, 1);
	memset((*buffer)+1, position, 1);
	memset((*buffer)+2, error, 1);
	memset((*buffer)+3, modifier, 1);
	memset((*buffer)+4, command, 1);
	memset((*buffer)+5, game_number, 1);
	memset((*buffer)+6, sequence, 1);		
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


int update_seq(Client *current, uint8_t *buffer)
{

	if(buffer == NULL || (buffer+1) == NULL || (buffer+2) == NULL ||  (buffer+3) == NULL ||  (buffer+4) == NULL || (buffer+5) == NULL || (buffer+6) == NULL)
	{
		return FAIL;
	}
	

	if((current->sequence_number + 1) >= MAX_SEQUENCE)
	{
		if(current->sequence_number == MAX_SEQUENCE)
		{
			current->sequence_number = MIN_SEQUENCE + 1;
		} else if ((current->sequence_number + 1) == MAX_SEQUENCE)
		{
			current->sequence_number = MIN_SEQUENCE;
		}

	} else
	{
		current->sequence_number = current->sequence_number + 2;
	}
	

	return SUCCESS;
}


int seq_check(Client *current, uint8_t *buffer)
{
	

	if(buffer == NULL || (buffer+1) == NULL || (buffer+2) == NULL ||  (buffer+3) == NULL ||  (buffer+4) == NULL || (buffer+5) == NULL || (buffer+6) == NULL)
	{
		return FAIL;
	} else 
	{
		if(current->sequence_number == MIN_SEQUENCE)
		{
			if(buffer[SEQUENCE] != MAX_SEQUENCE)
			{
				return FAIL;
			}
		} else 
		{
			if(buffer[SEQUENCE] != (current->sequence_number - 1))
			{
				return FAIL;
			}
		}
	}
	
	return SUCCESS;
	
}
