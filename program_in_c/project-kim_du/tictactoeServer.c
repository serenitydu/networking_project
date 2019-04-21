/**********************************************************/
/* This program is a TCP version of tictactoe      */
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
#include <netinet/ip.h>
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
		printf("ERROR: wrong number of arguments.\n");
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
	struct timeval time_out = {1, 0};
	struct ip_mreq mreq;
	memset(sd_list, 0, sizeof(int) * NUMBER_OF_CLIENTS);
	memset(client_list, 0, sizeof(struct sockaddr_in) * NUMBER_OF_CLIENTS);

	struct sockaddr_in server_addr;
	struct sockaddr_in addr;
	Client *list = NULL;
	Game_Number *list_numbers = NULL;
	fd_set fds;

	uint8_t *buffer = (uint8_t*) malloc(BUFFER_SIZE * sizeof(uint8_t));

	if(buffer == NULL)
	{
		printf("Failed to allocate a memory space.\n");
		return 0;
	}

	printf("The port number: %s\n",port);

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(port));
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
	/*create socket in TCP mode*/
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
		printf("ERROR: failed to create a socket.\n");
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

	int multi_sd;

		/*create socket in TCP mode*/
	if ((multi_sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
		printf("ERROR: failed to create a socket.\n");
		exit(0); 
  	} 
		
	/*Bind the addr*/
	bzero((char *)&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(MC_PORT); 

	if (bind(multi_sd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
	  perror("bind");
	  exit(1);
	}

	//from the lecture slide of our CSE5462 session.
	mreq.imr_multiaddr.s_addr = inet_addr(MC_GROUP);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);


	if(setsockopt(multi_sd, SOL_SOCKET, SO_RCVTIMEO, &time_out, sizeof(time_out)) < 0)
	{
		printf("ERROR: time out setting failed.\n");
	}

	if(setsockopt(multi_sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
	{
		printf("ERROR: failed to set multicast.\n");
	}


	initialize_game_numbers(&list_numbers, NUMBER_OF_CLIENTS);

	listen(sd, 1);
	/*Keep listen the incoming client*/
	while(1)
  	{
		int new_sd;
		int received = 0;
		struct sockaddr_in client;
		length = sizeof(client);

		FD_ZERO(&fds);
		FD_SET(sd, &fds);
		memset(buffer, 0, BUFFER_SIZE);
		
		/*Multicast handling.*/

		received = recvfrom(multi_sd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &client, &length);
		if(received > 0)
		{
			printf("REPORT: multicast message from a client.\n");
			if(received > 1 && buffer[0] == VERSION && buffer[1] == 1)
			{
				printf("REPORT: it is a correct message.\n");
				printf("%d %d\n", buffer[0], buffer[1]);
				memset(buffer, 0, BUFFER_SIZE);
				uint16_t port_for_tcp = htons(atoi(port));
				buffer[0] = VERSION;
				buffer[1] = 2;
				memcpy(buffer+2, &port_for_tcp, 2);
				if(current_number_of_clients < NUMBER_OF_CLIENTS)
				{
					printf("REPORT: sending a multicast reply.\n");
					sendto(multi_sd, buffer, 4, NONE,(struct sockaddr*)&(client), sizeof(client));
					printf("%d %d %d %d\n", buffer[0], buffer[1], buffer[2], buffer[3]);
					memcpy(&port_for_tcp,buffer+2,2);
					printf("%d\n", ntohs(port_for_tcp));
				}

			} else 
			{
				printf("ERROR: wrong message from udp multicast.\nLength: %d\n", received);
				int i = 0;
				for(i = 0; i < received; i++)
				{
					printf("%d \n", buffer[i]);
				}
			}
		}

		memset(buffer, 0, sizeof(uint8_t) * BUFFER_SIZE);

		for(i = 0; i < NUMBER_OF_CLIENTS; i++)
		{
			if(sd_list[i] > 0)
			{
				FD_SET(sd_list[i], &fds);
				if(sd_list[i] > maximum_sd)
					maximum_sd = sd_list[i];
			}
		}

		select(maximum_sd + 1, &fds, NULL, NULL, &time_out);
			
		if(FD_ISSET(sd, &fds))
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
			} else 
			{
				memset(buffer, 0, sizeof(uint8_t) * BUFFER_SIZE);
				read(new_sd, buffer, BUFFER_SIZE);
				printf("Already reached the maximum number of clients.\n");
				encode(&buffer, VERSION, NONE, GENERAL_ERROR, OUT_OF_RESOURCES, buffer[4], NONE, buffer[6]+1);
				write(new_sd,buffer,BUFFER_SIZE);
				printf("Sending: %u %u %u %u %u %u %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
				close(new_sd);
				memset(buffer, 0, sizeof(uint8_t) * BUFFER_SIZE);
			}
		}

		/*Receiving messages from the clients*/

		for(i = 0; i < NUMBER_OF_CLIENTS; i++)
		{
			if(sd_list[i] != 0 && FD_ISSET(sd_list[i], &fds))
			{
				printf("Waiting for the data.\n");
				int recv_length = read(sd_list[i], buffer, BUFFER_SIZE);
				
				if(recv_length <= 0 )
				{
					printf("Connection Lost.\n");
					
					Client *current = find_client(&list, client_list[i], current_number_of_clients);
					if(current != NULL)
						delete_client(&list, current, &list_numbers, &current_number_of_clients);

					close(sd_list[i]);
					sd_list[i] = 0;
					memset(client_list+i, 0, sizeof(struct sockaddr_in));
				} else 
				{
					printf("Just received the data.\n");
					printf("Receive: \nVERSION: %u\nPOSITION: %u\nSTATE: %u\nMODIFIER: %u\nCOMMAND: %u\nGAME NUMBER: %u\nSEQUENCE NUMBER: %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
					if(read_tictactoe(&list, &list_numbers, &current_number_of_clients, sd_list[i], buffer, client_list[i]) == -1){
						Client *current = find_client(&list, client_list[i], current_number_of_clients);
						if(current != NULL)
							delete_client(&list, current, &list_numbers, &current_number_of_clients);
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
	char mark = 'O'; 

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
				printf("ERROR: game is completed but the client claims it did not.\n");
				encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, MOVE, current->game->number, current->sequence_number);
				write(sd,buffer,BUFFER_SIZE);
				printf("Sending\nVERSION: %u\nPOSITION-NONE: %u\nSTATE-GENERAL ERROR: %u\nMODIFIER-INVALID REQUEST: %u\nCOMMAND-MOVE: %u\nGAME NUMBER: %u\nSEQUENCE NUMBER: %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
				return -1;
			}

			state_check = 1;
			if(checkwin(current->board) == 1)
			{
				/* Client wins */
				modifier = CLIENT_WINS;	
				if(buffer[3] != CLIENT_WINS)
				{
					printf("ERROR: the client claims the client did not win.\n");
					encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, END_GAME, current->game->number, current->sequence_number);
					write(sd,buffer,BUFFER_SIZE);
					printf("Sending\nVERSION: %u\nPOSITION-NONE: %u\nSTATE-GENERAL ERROR: %u\nMODIFIER-INVALID REQUEST: %u\nCOMMAND-END GAME: %u\nGAME NUMBER: %u\nSEQUENCE NUMBER: %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
					return -1;
				} else if(checkwin(current->board) == 0)
				{
					/* Client draws*/
					modifier = DRAW;
					if(buffer[3] != DRAW)
					{
						printf("ERROR: the client claims the game is not drawed.\n");
						encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, END_GAME, current->game->number, current->sequence_number);
						write(sd,buffer,BUFFER_SIZE);
						printf("Sending\nVERSION: %u\nPOSITION-NONE: %u\nSTATE-GENERAL ERROR: %u\nMODIFIER-INVALID REQUEST: %u\nCOMMAND-END GAME: %u\nGAME NUMBER: %u\nSEQUENCE NUMBER: %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
						return -1;
					}
				} 

				encode(&buffer, VERSION, NONE, GAME_COMPLETE, modifier, END_GAME, current->game->number, current->sequence_number);
				write(sd,buffer,BUFFER_SIZE);
				printf("Sending\nVERSION: %u\nPOSITION-NONE: %u\nSTATE-GAME COMPLETE: %u\nMODIFIER: %u\nCOMMAND-END GAME: %u\nGAME NUMBER: %u\nSEQUENCE NUMBER: %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
				update_seq(current, buffer);
				return -1;
			} else 
			{
				/* Game in progress */
				if(buffer[2] == GAME_COMPLETE)
				{
					printf("ERROR: the game is still in progress but the client claims it is ended.\n");
					encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, MOVE, current->game->number, current->sequence_number);
					write(sd,buffer,BUFFER_SIZE);
					printf("Sending\nVERSION: %u\nPOSITION-NONE: %u\nSTATE-GENERAL ERROR: %u\nMODIFIER-INVALID REQUEST: %u\nCOMMAND-MOVE: %u\nGAME NUMBER: %u\nSEQUENCE NUMBER: %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
					update_seq(current, buffer);
					return -1;
				}
			}

		}
	}
	else
	{
		printf("ERROR: invalid move for the client\n");
		encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, NONE, current->game->number, current->sequence_number);
		write(sd,buffer,BUFFER_SIZE);
		printf("Sending\nVERSION: %u\nPOSITION-NONE: %u\nSTATE-GENERAL ERROR: %u\nMODIFIER-INVALID REQUEST: %u\nCOMMAND: %u\nGAME NUMBER: %u\nSEQUENCE NUMBER: %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
		update_seq(current, buffer);
		return -1;
    	}
	
	/*Server do the choice*/
	state_check = server_move(current, sd, buffer, MOVE);

	if(state_check == GAME_COMPLETE)
	{
		return state_check;
	}

	return GAME_IN_PROGRESS;
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
		printf("ERROR: wrong sequence.\n");
		printf("Received: \nVERSION: %u\nPOSITION: %u\nSTATE: %u\nMODIFIER: %u\nCOMMAND: %u\nGAME NUMBER: %u\nSEQUENCE NUMBER: %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
		/*
		write(client_sd, buffer, BUFFER_SIZE);
		printf("ERROR: wrong sequence.\nSending: %u %u %u %u %u %u %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
		printf("Sending\nVERSION: %u\nPOSITION-NONE: %u\nSTATE: %u\nMODIFIER: %u\nCOMMAND: %u\nGAME NUMBER: %u\nSEQUENCE NUMBER: %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);*/
		return 0;
	
	}

	//Sequence number update.
	if(buffer[6] == MAX_SEQUENCE)
	{
		sequence = MIN_SEQUENCE;
	} else 
	{
		sequence = buffer[6] + 1;
	}
	

	if (buffer[0] != VERSION)
	{
		printf("ERROR: just received a message of a wrong version number.\n");
		
		//case when the version number is incorrect.
		//checking if the sender is our client.
		if(current != NULL)
		{
			encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, NONE, NONE, current->sequence_number);
			write(client_sd,buffer,BUFFER_SIZE);

			printf("ERROR: client-wrong version\n");
			printf("Sending\nVERSION: %u\nPOSITION-NONE: %u\nSTATE-GENERAL ERROR: %u\nMODIFIER-INVALID REQUEST: %u\nCOMMAND: %u\nGAME NUMBER: %u\nSEQUENCE NUMBER: %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
			update_seq(current, buffer);
		} else 
		{

			encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, NONE, NONE, sequence);
			write(client_sd,buffer,BUFFER_SIZE);
			printf("ERROR: not client-wrong version\n");
			printf("Sending\nVERSION: %u\nPOSITION-NONE: %u\nSTATE-GENERAL ERROR: %u\nMODIFIER-INVALID REQUEST: %u\nCOMMAND: %u\nGAME NUMBER: %u\nSEQUENCE NUMBER: %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
		}


		return 0;
	}
	
	if(buffer[4] == RECONNECT)
	{
		//RECONNECT message.
		//checking if the sender is our client.
		if(current != NULL)
		{
			encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, RECONNECT, current->game->number, current->sequence_number);
			write(client_sd, buffer, BUFFER_SIZE);
			printf("ERROR-RECONNECT FAILED\nSending:\nVERSION: %u\nPOSITION-NONE: %u\nSTATE-GENERAL_ERROR: %u\nMODIFIER-INVALID REQUEST: %uCOMMAND_RECONNECT: %u\nGAME NUMBER: %u\nSEQUENCE NUMBER: %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
			delete_client(list, current, list_numbers, current_number_of_clients);
			return -1;
		} else 
		{
			if(*current_number_of_clients < NUMBER_OF_CLIENTS)
			{
				current = insert_client(list, client_structure, list_numbers, current_number_of_clients, buffer, RECONNECT);
				if(current != NULL)
				{
					printf("REPORT: reconnect client added.\n");
					server_move(current, client_sd, buffer, RECONNECT);
					print_clients(*list, *current_number_of_clients);
				} else
				{
					printf("ERROR: failed to allocate a memory space for a client.\n");
					encode(&buffer, VERSION, NONE, GENERAL_ERROR, OUT_OF_RESOURCES, RECONNECT, NONE, sequence);
					write(client_sd,buffer,BUFFER_SIZE);
					printf("RECONNECT\nFAIL\nSending:\nVERSION: %u\nPOSITION-NONE: %u\nSTATE-GENERAL ERROR: %u\nMODIFIER-OUT OF RESOURCES: %u\nCOMMAND-RECONNECT: %u\nGAME NUMBER-NONE: %u\nSEQUENCE NUMBER: %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
				}
			} else 
			{	
				printf("ERROR: not enough space for a new client.\n");
				encode(&buffer, VERSION, NONE, GENERAL_ERROR, OUT_OF_RESOURCES, RECONNECT, NONE, sequence);
				write(client_sd,buffer,BUFFER_SIZE);

				printf("Sending\nVERSION: %u\nPOSITION-NONE: %u\nSTATE-GENERAL ERROR: %u\nMODIFIER-OUT OF RESOURCES: %u\nCOMMAND-RECONNECT: %u\nGAME NUMBER: %u\nSEQUENCE NUMBER: %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
			}
		}
	}
	else if(buffer[4] == NEW_GAME)
	{
		//NEW GAME case.
		//checking if the sender is our client.
		if(current != NULL)
		{
			
			encode(&buffer, VERSION, NONE, GENERAL_ERROR, TRY_AGAIN, NEW_GAME, current->game->number, current->sequence_number);
			write(client_sd,buffer,BUFFER_SIZE);
			printf("ERROR\nNEW GAME-NOT CLIENT\n");

			printf("Sending\nVERSION: %u\nPOSITION-NONE: %u\nSTATE-GENERAL ERROR: %u\nMODIFIER-TRY AGAIN: %u\nCOMMAND-NEW GAME: %u\nGAME NUMBER: %u\nSEQUENCE NUMBER: %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);

			delete_client(list, current, list_numbers, current_number_of_clients);
			return -1;
		} else
		{
			if(*current_number_of_clients < NUMBER_OF_CLIENTS)
			{
				current = insert_client(list, client_structure, list_numbers, current_number_of_clients, buffer, NEW_GAME);
				if(current != NULL)
				{
					encode(&buffer, VERSION, NONE, NONE, NONE, NEW_GAME, current->game->number, current->sequence_number);
					write(client_sd,buffer,BUFFER_SIZE);
					printf("REPORT: NEW GAME-CLIENT\n");
					printf("Sending\nVERSION: %u\nPOSITION-NONE: %u\nSTATE: %u\nMODIFIER: %u\nCOMMAND-NEW GAME: %u\nGAME NUMBER: %u\nSEQUENCE NUMBER: %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);

					print_clients(*list, *current_number_of_clients);

					
					update_seq(current, buffer);
				} else
				{
					printf("ERROR: failed to allocate a memory space for a client.\n");
					encode(&buffer, VERSION, NONE, GENERAL_ERROR, OUT_OF_RESOURCES, RECONNECT, NONE, sequence);
					write(client_sd,buffer,BUFFER_SIZE);

					printf("Sending\nVERSION: %u\nPOSITION-NONE: %u\nSTATE-GENERAL ERROR: %u\nMODIFIER-OUT OF RESOURCES: %u\nCOMMAND-RECONNECT: %u\nGAME NUMBER: %u\nSEQUENCE NUMBER: %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
				}
			} else 
			{	
				printf("ERROR: not enough space for a new client.\n");
				encode(&buffer, VERSION, NONE, GENERAL_ERROR, OUT_OF_RESOURCES, NONE, NONE, sequence);
				write(client_sd,buffer,BUFFER_SIZE);
				printf("Sending\nVERSION: %u\nPOSITION-NONE: %u\nSTATE-GENERAL ERROR: %u\nMODIFIER-OUT_OF_RESOURCES: %u\nCOMMAND: %u\nGAME NUMBER: %u\nSEQUENCE NUMBER: %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
			}
		}

	} else if(buffer[4] == MOVE)
	{
		//Move Case.
		//checking if the sender is our client.
		if(current != NULL)
		{
			if(buffer[5] == current->game->number)
			{					
				int status = tictactoe(current, client_sd, buffer[1], buffer);
				if(status == -1)
				{
					delete_client(list, current, list_numbers, current_number_of_clients);
					return -1;
				}
							
			} else
			{
				if(buffer[5] >= 0 && buffer[5] < NUMBER_OF_CLIENTS)
				{
					printf("ERROR: WARNING\nPlayer at a game number of %d is trying to access a game number %d.\n", current->game->number, buffer[5]);
				} else 
				{
					printf("ERROR: player at a game number of %d is trying to access an invalid game number %d.\n", current->game->number, buffer[5]);
				}
				encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, NONE, NONE, current->sequence_number);
				write(client_sd,buffer,BUFFER_SIZE);
				printf("Sending\nVERSION: %u\nPOSITION-NONE: %u\nSTATE-GENERAL ERROR: %u\nMODIFIER-INVALID REQUEST: %u\nCOMMAND: %u\nGAME NUMBER: %u\nSEQUENCE NUMBER: %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
				update_seq(current, buffer);
			}
		} else
		{
			printf("ERROR: WARNING\nSomeone is faking his/her identity.\nNon-client requested for a move.\n");
		
			encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, NONE, NONE, sequence);
			write(client_sd,buffer,BUFFER_SIZE);
			printf("Sending\nVERSION: %u\nPOSITION-NONE: %u\nSTATE-GENERAL ERROR: %u\nMODIFIER-INVALID REQUEST: %u\nCOMMAND: %u\nGAME NUMBER: %u\nSEQUENCE NUMBER: %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
		}

	} else if(buffer[4] == END_GAME)
	{
		//checking if the sender is our client.
		if(current != NULL)
		{
			delete_client(list, current, list_numbers, current_number_of_clients);
			memset(buffer, 0, BUFFER_SIZE);
			return -1;
		} else 
		{
			printf("ERROR: not a client but wants to end the game.\n");
			encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, NONE, NONE, sequence);
			write(client_sd,buffer,BUFFER_SIZE);
			printf("Sending\nVERSION: %u\nPOSITION-NONE: %u\nSTATE-GENERAL ERROR: %u\nMODIFIER-INVALID REQUEST: %u\nCOMMAND: %u\nGAME NUMBER: %u\nSEQUENCE NUMBER: %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
		}	
	} else 
	{

		printf("ERROR: just received a message of a wrong command:%d.\n", buffer[4]);
		//Invalid command case.
		//checking if the sender is our client.
		if(current != NULL)
		{
			encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, MOVE, NONE, current->sequence_number);
			write(client_sd,buffer,BUFFER_SIZE);
			printf("Command invalid-client\n");
			printf("Sending\nVERSION: %u\nPOSITION-NONE: %u\nSTATE-GENERAL ERROR: %u\nMODIFIER-INVALID REQUEST: %u\nCOMMAND-MOVE: %u\nGAME NUMBER: %u\nSEQUENCE NUMBER: %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);

			update_seq(current, buffer);
		} else 
		{
			encode(&buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, MOVE, NONE, sequence);
			write(client_sd,buffer,BUFFER_SIZE);
			printf("Command invalid-not client\n");
			printf("Sending\nVERSION: %u\nPOSITION-NONE: %u\nSTATE-GENERAL ERROR: %u\nMODIFIER-INVALID REQUEST: %u\nCOMMAND-MOVE: %u\nGAME NUMBER: %u\nSEQUENCE NUMBER: %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);				
			
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

//a function to configure next move.
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
	printf("REPORT: removing the client.\n");
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
Client * insert_client(Client **list, struct sockaddr_in client, Game_Number **head, int *current_number_of_clients, uint8_t *buffer, int command)
{
	
	Client *target = NULL;
	target = malloc(sizeof(Client));
	int i = 0;

	if(target == NULL)
	{
		printf("ERROR: failed to allocate a memory space for a client.\n");
		return target;
	}

	printf("REPORT: a client is inserted.\n");
	target->sequence_number = buffer[6] + 1;
	target->next = NULL;
	target->prev = NULL;
	target->client = client;
	target->player_number = 2;

	initSharedState(target->board);

	if(command == RECONNECT)
	{
		for(i = 0; i < 9; i++)
		{
			if(buffer[BOARD + i] == CLIENT_SPACE)
			{
				target->board[i/3][i%3] = 'O';

			} else if(buffer[BOARD + i] == SERVER_SPACE)
			{
				target->board[i/3][i%3] = 'X';
			}
		}
	}

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

//Creating a list of game numbers to use.
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

//a function to get one of the game numbers.
Game_Number * get_game_number(Game_Number **head)
{
	Game_Number *target = NULL;
	target = *head;
	*head = (*head)->next;
	target->next = NULL;
	return target;
}

//a funtion to return a game number after using it.
void return_game_number(Game_Number **head, Game_Number *target)
{
	target->next = *head;
	*head = target;
}

//encoding the message.
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
	printf("REPORT: current player list:\n");
	for(i = 0; i < current_number_of_clients;i++)
	{
		printf("Game number: %u.\n", current->game->number);
		current = current->next;
	}
	printf("\n");
}

//a funcion to update a sequence.
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

//a function to validate a sequence number.
int seq_check(Client *current, uint8_t *buffer)
{
	

	if((buffer+SEQUENCE) == NULL)
	{
		return FAIL;
	} else 
	{
		printf("Current sequence %d\n", current->sequence_number);
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

//a function to configure a next move for the server.
//send the move to the client.
int server_move(Client *current, int sd, uint8_t *buffer, int reconnect)
{
	char mark;
	uint8_t choice;
	int row, column;

	/*Server do the choice*/
	choice = next_move(current->board);
	mark = 'X'; 

	row = (int)((choice-1) / ROWS); 
	column = (choice-1) % COLUMNS;
	current->board[row][column] = mark;
	/*Check if client win*/
	if(checkwin(current->board) != -1)
	{
		if(checkwin(current->board) == 1)
		{
			/* Server win */
			encode(&buffer, VERSION, choice, GAME_COMPLETE, SERVER_WIN, MOVE, current->game->number, current->sequence_number);
			write(sd,buffer,BUFFER_SIZE);
			printf("FINAL MOVE\nSending\nVERSION: %u\nPOSITION: %u\nSTATE-GAME COMPLETE: %u\nMODIFIER-SERVER WINS: %u\nCOMMAND_MOVE: %u\nGAME NUMBER: %u\nSEQUENCE NUMBER: %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
		} else 
		{
			/* Draw */
			encode(&buffer, VERSION, choice, GAME_COMPLETE, DRAW, MOVE, current->game->number, current->sequence_number);
			write(sd,buffer,BUFFER_SIZE);
			printf("FINAL MOVE\nSending\nVERSION: %u\nPOSITION: %u\nSTATE-GAME COMPLETE: %u\nMODIFIER-DRAW: %u\nCOMMAND_MOVE: %u\nGAME NUMBER: %u\nSEQUENCE NUMBER: %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
		}

		update_seq(current, buffer);
		return GAME_COMPLETE;
	} else 
	{
		if(reconnect == RECONNECT)
		{
			encode(&buffer, VERSION, choice, GAME_IN_PROGRESS, NONE, RECONNECT, current->game->number, current->sequence_number);
			write(sd,buffer,BUFFER_SIZE);
			printf("NON-FINAL MOVE\nSending\nVERSION: %u\nPOSITION: %u\nSTATE-GAME IN PROGRESS: %u\nMODIFIER-NONE: %u\nCOMMAND_RECONNECT: %u\nGAME NUMBER: %u\nSEQUENCE NUMBER: %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
		} else 
		{
			encode(&buffer, VERSION, choice, GAME_IN_PROGRESS, NONE, MOVE, current->game->number, current->sequence_number);
			write(sd,buffer,BUFFER_SIZE);
			printf("NON-FINAL MOVE\nSending\nVERSION: %u\nPOSITION: %u\nSTATE-GAME IN PROGRESS: %u\nMODIFIER-NONE: %u\nCOMMAND_MOVE: %u\nGAME NUMBER: %u\nSEQUENCE NUMBER: %u\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
		}
		update_seq(current, buffer);
	}

	return GAME_IN_PROGRESS;

}
