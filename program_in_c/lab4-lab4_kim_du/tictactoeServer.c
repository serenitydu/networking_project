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

/* #define section, for now we will define the number of rows and columns */
#define ROWS  3
#define COLUMNS  3
#define BUFFER_SIZE 1000
#define DATAGRAM_SIZE 4
#define SERVER_NUMBER 1
#define CLIENT_NUMBER 2
#define VERSION 3

/* C language requires that you predefine all the routines you are writing */
int checkwin(char board[ROWS][COLUMNS]);
void print_board(char board[ROWS][COLUMNS]);
int tictactoe(char board[ROWS][COLUMNS], int player_number, int sd);
int initSharedState(char board[ROWS][COLUMNS]);
int client(char *ip_addr, char *port);
int server(char *port);

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
	char board[ROWS][COLUMNS]; /*Creating a board.*/
	struct sockaddr_in server_addr;
	struct timeval time_out = {15,0};

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

	/*setting a timeout*/
	setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &time_out, sizeof(time_out));

	/*bind the socket with ip address and port*/
	if(bind(sd,(struct sockaddr*)&server_addr,sizeof(struct sockaddr))<0)
	{
        	printf("ERROR: Bind error\n");
		close(sd);
        	return -1;
   	}
	

	/*Keep listen the incoming client*/
	while(1)
  	{
			initSharedState(board); // Initialize the 'game' board
			tictactoe(board, SERVER_NUMBER, sd); // call the 'game'

	}

	close(sd);    
	return 0;
}

int tictactoe(char board[ROWS][COLUMNS], int player_number, int sd)
{
	/* Modify the original tictactoe game to a networking version */

	char mark;      // either an 'x' or an 'o'
	int player = 2; // keep track of whose turn it is
	int i;  // used for keeping track of choice user makes
	int row, column;
	int win_check; // used to be check if the final results match between the client and the server.

	/* Define the three bytes for transfer protocal */
	uint8_t choice;
	uint8_t state_check;
	uint8_t *buffer;

	/*Creating a sockaddr_in structure for a client.*/
	struct sockaddr_in client;
	socklen_t address_length = sizeof(client);
	memset(&client, 0, address_length);

	/* loop, first print the board, then ask player 'n' to make a move */

	do {
		win_check = 0;
		choice = 0;
		print_board(board); // call function to print the board on the screen
		player = (player % 2) ? 1 : 2;  // Mod math to figure out who the player is

    		buffer = (uint8_t*) calloc(BUFFER_SIZE, sizeof(uint8_t));

		if(player== player_number)
		{
			/* print out player so you can pass game */
			printf("Player %d, enter a number:  ", player); 
			/* using scanf to get the choice */
			scanf("%"SCNu8, &choice);
		} 
    		else
		{
			int recv_length = 0;
			//handle UDP

			printf("Waiting for the data.\n");

			/*Receiving a data from the client.*/
			recv_length = recvfrom(sd, buffer, DATAGRAM_SIZE, 0, (struct sockaddr*) &client, &address_length);

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
			/* Check the protocal version */
			if(buffer[0] != VERSION)
			{
				printf("Wrong version message: %u.\n", buffer[0]);
				free(buffer);
				return 0;
			} 
			/* Check the game state*/
            		else if (buffer[2] == 2)
			{
				printf("There was an error.\n");
				free(buffer);
				return 0;

			}
			/* Check if there was an error*/	
			else if (buffer[2] != 0 && buffer[2] != 1) 
			{
				printf("Received an invalid message.\n");
				free(buffer);
				return 0;
			} 
			/*Game complete check*/
			else if (buffer[2] == 1)
			{
				win_check = buffer[3];
			}

			memcpy(&choice, &buffer[1], 1);
		}

		mark = (player == 1) ? 'X' : 'O'; //depending on who the player is, either us x or o
		/******************************************************************/
		/** little math here. you know the squares are numbered 1-9, but  */
		/* the program is using 3 rows and 3 columns. We have to do some  */
		/* simple math to conver a 1-9 to the right row/column            */
		/******************************************************************/
		row = (int)((choice-1) / ROWS); 
		column = (choice-1) % COLUMNS;

		/* first check to see if the row/column chosen is has a digit in it, if it */
		/* square 8 has and '8' then it is a valid choice                          */

		if (board[row][column] == (choice+'0'))
		{
			board[row][column] = mark;
			if(player == player_number)
			{
				uint8_t modifier = 0;
				memset(buffer, VERSION, 1); //Set version number
				memset(buffer+1, choice, 1);

				
				if(checkwin(board) != -1)
				{
            				state_check = 1;

					if(checkwin(board) == 1)
					{
						modifier = 2;
					} else {
						modifier = 1;
					}
				} else 
				{
            				state_check = 0;
				}

				memset(buffer+2, state_check, 1);
				memset(buffer+3, modifier, 1);

				printf("Sending protocal: %u %u %u %u\n", buffer[0], buffer[1], buffer[2],buffer[3]);
				int check = sendto(sd, buffer, DATAGRAM_SIZE, 0,(struct sockaddr*)&client, sizeof(client));
				printf("Sending size: %d",check);

				if(check < 4)
				{
					
					printf("Failed to write.\n");
					free(buffer);
					return 0;
				}

			} 
			else 
			{
				memset(buffer, VERSION, 1); //Set version number
				memset(buffer+1, 0, 1);

				//Checking if a client's claim for an end is correct.
				if(checkwin(board) != -1 && checkwin(board) != win_check)
				{
					state_check = 2;
					printf("ERROR: results from both users are not same.\n");
				}

				if(state_check == 2)
				{
					
					//There was an error for the client's data. Send a error message to the client.
					memset(buffer+2, state_check, 1);
					int check = sendto(sd, buffer, DATAGRAM_SIZE, 0,(struct sockaddr*)&client,sizeof(client));
					printf("%d",check);
					if(check < 3)
					{
					
						printf("Failed to write.\n");
						free(buffer);
						return 0;
					}
				}
			}
		}
		else
		{
			printf("Invalid move ");
			if(player == player_number)
			{
				player--;
				getchar();
			} else 
			{

				printf("from other user.\n Ending the game ..\n");				
				memset(buffer, VERSION, 1); //Set version number;
				
				memset(buffer+2, 2, 1);


				if(sendto(sd, buffer, BUFFER_SIZE, 0,(struct sockaddr*)&client, sizeof(client)) < 3)
				{
					printf("Failed to write.\n");
					free(buffer);
					return 0;
				}

				free(buffer);
				return 0;
			}
      		}

		/* after a move, check to see if someone won! (or if there is a draw */
		i = checkwin(board);
		player++;

		free(buffer);

	} while (i ==  -1); // -1 means no one won
    
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
