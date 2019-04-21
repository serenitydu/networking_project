/**********************************************************/
/* This program is a UDP version of tictactoe      				*/
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
#include <sys/time.h>
#include <sys/types.h>
#include "tictactoe.h"


int main(int argc, char *argv[])
{
	/*check arguments*/
	/*If user input 3 arguments, call client function*/
	if(argc == 4)
	{
		if(atoi(argv[2]) == 2)
		{
			char *ip_addr = argv[3];
			char *port = argv[1];
			client(ip_addr,port);
			printf("The server IP address: %s\n",ip_addr);
			printf("The port number: %s\n",port);

		} else 
		{
			printf("The player number for a client is 2.\n");
		}
	}else 
	{
		printf("Wrong number of arguments.\n");
		printf("./tictactoeOriginal <port> <player number> <server ip> for a client.\n");
	}		
  
	return 0; 
}


int client(char *ip_addr, char *port)
{

	uint8_t *recv_buffer = NULL;
	uint8_t *send_buffer = NULL;
	uint8_t new_game = 0;	//Fifth byte
	uint8_t game_num = 0;	//Sixth byte
	socklen_t address_length;
	int sd = 0;
	char board[ROWS][COLUMNS];
	struct sockaddr_in to_server;
	struct timeval timeout={15,0}; //Set timeout interval


	address_length = sizeof(to_server);
	/* assigning values for sockaddr_in structure. */
	to_server.sin_family = AF_INET;
	to_server.sin_port = htons(atoi(port));
	to_server.sin_addr.s_addr = inet_addr(ip_addr);
    
 	sd = socket(AF_INET,SOCK_DGRAM,0);

	/* socket error check. */
	if(sd < 0)
	{
		printf("Failed to create a socket.\n");
		exit(1);
	}
	/* Bind timeout with the socket*/
	setsockopt(sd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(struct timeval));
	printf("Created a socket.\n");

	printf("Do you want to start a new game? (0: yes, 1: No)\n"); 
	/* using scanf to get the choice */
	scanf("%"SCNu8, &new_game);

	printf("%p %p\n", recv_buffer, send_buffer);
	recv_buffer = (uint8_t*)malloc(BUFFER_SIZE * sizeof(uint8_t));
	send_buffer = (uint8_t*)malloc(DATAGRAM_SIZE * sizeof(uint8_t));

	while(new_game == NEW_GAME){
        // Initialize the 'game'
		memset(recv_buffer, 0, BUFFER_SIZE);
		memset(send_buffer, 0, DATAGRAM_SIZE);

		initSharedState(board); 
		printf("Sending message for the handshake.....\n");
		encode(&send_buffer, VERSION, NONE, NONE, NONE, NEW_GAME, NONE);
		sendto(sd, send_buffer, DATAGRAM_SIZE, 0,(struct sockaddr*)&to_server,sizeof(to_server));
		printf("Sent the request.\n");

		int recv_length = recvfrom(sd, recv_buffer, DATAGRAM_SIZE, 0, (struct sockaddr*) &to_server, &address_length);
		printf("%u %u %u %u %u %u\n", recv_buffer[0], recv_buffer[1], recv_buffer[2], recv_buffer[3], recv_buffer[4], recv_buffer[5]);

		if(recv_length < 0 )
		{
			printf("Faild to receive a data.\n");
			break;
		} 
		else if (recv_length == 0) 
		{
			printf("Conenction Lost.\n");
			break;
		} 
		/* Check the protocal version */
		if(recv_buffer[0] != VERSION)
		{
			printf("Wrong version message: %u.\n", recv_buffer[0]);
			break;

		} 
		/* Check the game state*/
		else if (recv_buffer[2] == GENERAL_ERROR)
		{
			if(recv_buffer[3] == OUT_OF_RESOURCES){
				printf("Server out of resources.\n");
			}else if(recv_buffer[3] == INVALID_REQUEST){
				printf("Client invalid request.\n");
			}else if(recv_buffer[3] == SHUT_DOWN){
				printf("Server shut down.\n");
			}else if(recv_buffer[3] == TIME_OUT){
				printf("Client game timeout.\n");
			}else if(recv_buffer[3] == TRY_AGAIN){
				printf("Wrong request,try again.\n");
			}
			break;

		}
       				
		game_num = recv_buffer[5];

		printf("Game started, your game number is: %u.\n",game_num); 
		memset(send_buffer+5, game_num, 1); //Set game number
		new_game = 1;
		memset(send_buffer+4, NEW_GAME, 1); //Set game number
		tictactoe_client(board, sd, to_server, recv_buffer, send_buffer); // call the 'game' 	
		recvfrom(sd, recv_buffer, DATAGRAM_SIZE, 0, (struct sockaddr*) &to_server, &address_length);
		printf("Do you want to start a new game? (0: yes, 1: No)\n"); 
		/* using scanf to get the choice */
		scanf("%"SCNu8, &new_game);
		

		
	}
	printf("I want to free them.\n");
	printf("%p %p\n", recv_buffer, send_buffer);
	printf("%u %u\n", *recv_buffer, *send_buffer);
	free((uint8_t*)send_buffer);
	printf("Freeing second one.\n");

	free((uint8_t*)recv_buffer);
	printf("I want to close.\n");
	close(sd);

	return 0;
}


int tictactoe_client(char board[ROWS][COLUMNS], int sd, struct sockaddr_in to_server,uint8_t *recv_buffer,uint8_t *send_buffer)
{
	/* Modify the original tictactoe game to a networking version */


	/* Define the three bytes for transfer protocal */
	uint8_t choice;				//second byte
	uint8_t state_check;	//second byte
	uint8_t modifier = 1;	//Forth byte

	char mark;      // either an 'x' or an 'o'
	int player = 2; // keep track of whose turn it is
	int i;  // used for keeping track of choice user makes
	int row, column, win_check;

	socklen_t address_length = sizeof(to_server);
	
	/* loop, first print the board, then ask player 'n' to make a move */
	do {
		choice = 0;
		print_board(board); // call function to print the board on the screen
		player = (player % 2) ? 1 : 2;  // Mod math to figure out who the player is

		if(player== CLIENT_NUMBER)
		{
			/* print out player so you can pass game */
			printf("Player %d, enter a number:  ", player); 
			/* using scanf to get the choice */
			scanf("%"SCNu8, &choice);
		}else
		{
			int recv_length = 0;
			
			//handle UDP
			printf("Waiting for server playing...\n");
			recv_length = recvfrom(sd, recv_buffer, DATAGRAM_SIZE, 0, (struct sockaddr*) &to_server, &address_length);
			printf("Just received the data, size: %d.\n",recv_length);
			/* Check if receive data correctly. */
			if(recv_length < 0 )
			{
				printf("Faild to receive a data.\n");
				return 0;
			} 
			else if (recv_length == 0) 
			{
				printf("Conenction Lost.\n");
				return 0;
			} 
			/* Check the protocal version */
			if(recv_buffer[0] != VERSION)
			{
				printf("Wrong version message: %u.\n", recv_buffer[0]);
				return 0;
			} 
			/* Check the game state*/
     			else if (recv_buffer[2] == GENERAL_ERROR)
			{
				printf("General error.\n");
				if(recv_buffer[3] == INVALID_REQUEST)
				{
					printf("Invalid request.\n");
				}
				else if(recv_buffer[3] == SHUT_DOWN)
				{
					printf("Server shutdown.\n");
				}
				else if(recv_buffer[3] == TIME_OUT)
				{
					printf("Client game timeout.\n");
				}
				else if(recv_buffer[3] == TRY_AGAIN)
				{
					printf("Try again.\n");
				}
				return 0;

			}
			/* Check if there was an error*/	
			else if (recv_buffer[2] != GAME_IN_PROGRESS && recv_buffer[2] != GAME_COMPLETE) 
			{
				printf("Received an invalid message.\n");
				return 0;
			} 
			/*Game complete check*/
			else if (recv_buffer[2] == GAME_COMPLETE)
			{
				win_check = recv_buffer[3];
			}
			choice = recv_buffer[1];
		}

		mark = (player == 1) ? 'X' : 'O'; //depending on who the player is, either us x or o

		row = (int)((choice-1) / ROWS); 
		column = (choice-1) % COLUMNS;

		/* first check to see if the row/column chosen is has a digit in it, if it */
		/* square 8 has and '8' then it is a valid choice                          */

		if (board[row][column] == (choice+'0'))
		{
			board[row][column] = mark;
			if(player == CLIENT_NUMBER)
			{

				if(checkwin(board) != -1)
				{
            				state_check = 1;
					if(checkwin(board) == 1)
					{
						/* I win */
						modifier = 2;
					} else 
					{
						/* Draw */
						modifier = 1;
					}
				} else 
				{
          				state_check = 0;
				}


				printf("The server IP address: %s\n", inet_ntoa(to_server.sin_addr));
				printf("The port number: %u\n", ntohs(to_server.sin_port));
				printf("Sending protocal: %u %u %u %u %u %u\n", send_buffer[0], send_buffer[1], send_buffer[2],send_buffer[3],send_buffer[4],send_buffer[5]);

				encode(&send_buffer, VERSION, choice, state_check, modifier, MOVE, send_buffer[5]); 
				int check = sendto(sd, send_buffer, DATAGRAM_SIZE, 0,(struct sockaddr*)&to_server,sizeof(to_server));
				printf("Sending size: %d\n",check);
				
				if(check < 6)
				{
					printf("Failed to write.\n");
					return 0;
				}

			} else 
			{

				memset(send_buffer+1, 0, 1);
				
				if(checkwin(board) != -1 && checkwin(board) != win_check)
				{
					state_check = 2;
					printf("ERROR: results from both users are not same.\n");
				}

				if(state_check == GENERAL_ERROR)
				{

					memset(send_buffer+2, state_check, 1);

					encode(&send_buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, MOVE, send_buffer[5]);
					int check = sendto(sd, send_buffer, DATAGRAM_SIZE, 0,(struct sockaddr*)&to_server,address_length);
					printf("%d",check);
					if(check < DATAGRAM_SIZE)
					{
						printf("Failed to write.\n");
						return 0;
					}
				}
			}

		}else
		{
			printf("Invalid move\n");
			if(player == CLIENT_NUMBER)
			{
				player--;
				getchar();
			} else 
			{
        			state_check = GENERAL_ERROR;
				
				encode(&send_buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, MOVE, send_buffer[5]);
				if(sendto(sd, send_buffer, DATAGRAM_SIZE, 0,(struct sockaddr*)&to_server,sizeof(to_server)) < 4)
				{
					printf("Failed to write.\n");
					return 0;
				}

				return 0;
			}
    		}

		/* after a move, check to see if someone won! (or if there is a draw */
		i = checkwin(board);
		player++;


	} while (i == -1); // -1 means no one won
    
	/* print out the board again */
  	print_board(board);
    
  	if (i == 1) // means a player won!! congratulate them
	{
		printf("==>\aPlayer %d wins\n ", --player);
	}
	else
	{
		printf("==>\aGame draw"); // ran out of squares, it is a draw
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

int initSharedState(char board[ROWS][COLUMNS])
{    
    /* this just initializing the shared state aka the board */
    int i, j, count = 1;
    printf ("in sharedstate area\n");
    for (i=0;i<3;i++)
        for (j=0;j<3;j++){
            board[i][j] = count + '0';
            count++;
        }
    return 0;
}


void encode(uint8_t **buffer, uint8_t version, uint8_t position, uint8_t error, uint8_t modifier, uint8_t command, uint8_t game_number)
{
	memset(*buffer, version, 1);
	memset((*buffer)+1, position, 1);
	memset((*buffer)+2, error, 1);
	memset((*buffer)+3, modifier, 1);
	memset((*buffer)+4, command, 1);
	memset((*buffer)+5, game_number, 1);	
}

