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
/* #define section, for now we will define the number of rows and columns */
#define ROWS  3
#define COLUMNS  3
#define BUFFER_SIZE 1000
#define DATAGRAM_SIZE 4
#define CLIENT_NUMBER 2
#define VERSION 3

/* C language requires that you predefine all the routines you are writing */
int checkwin(char board[ROWS][COLUMNS]);
void print_board(char board[ROWS][COLUMNS]);
int tictactoe(char board[ROWS][COLUMNS], int player_number, int sd, struct sockaddr_in to_server);
int initSharedState(char board[ROWS][COLUMNS]);
int client(char *ip_addr, char *port);


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
	char board[ROWS][COLUMNS];

	printf("The server IP address: %s\n",ip_addr);
	printf("The port number: %s\n",port);

	int sd = 0;
	struct sockaddr_in to_server;
	struct timeval timeout={15,0}; //Set timeout interval

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

	initSharedState(board); // Initialize the 'game' board
	tictactoe(board, CLIENT_NUMBER, sd, to_server); // call the 'game' 	
	close(sd);

	return 0;
}


int tictactoe(char board[ROWS][COLUMNS], int player_number, int sd, struct sockaddr_in to_server)
{
	/* Modify the original tictactoe game to a networking version */

	char mark;      // either an 'x' or an 'o'
	int player = 2; // keep track of whose turn it is
	int i;  // used for keeping track of choice user makes
	int row, column, win_check;

	/* Define the three bytes for transfer protocal */
	uint8_t choice;
	uint8_t state_check;
	uint8_t modifier = 1;
	uint8_t *buffer;
	socklen_t address_length = sizeof(to_server);

	/* loop, first print the board, then ask player 'n' to make a move */

	do {
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
		}else
		{
			int recv_length = 0;
			
			//handle UDP
			printf("Waiting for the data.\n");
			recv_length = recvfrom(sd, buffer, DATAGRAM_SIZE, 0, (struct sockaddr*) &to_server, &address_length);

			printf("Just received the data, size: %d.\n",recv_length);
			if(recv_length < 0 )
			{
				printf("Faild to receive a data.\n");
				free(buffer);
				return 0;
			} 
			else if (recv_length == 0) 
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

		row = (int)((choice-1) / ROWS); 
		column = (choice-1) % COLUMNS;

		/* first check to see if the row/column chosen is has a digit in it, if it */
		/* square 8 has and '8' then it is a valid choice                          */

		if (board[row][column] == (choice+'0'))
		{
			board[row][column] = mark;
			if(player == player_number)
			{
				memset(buffer, VERSION, 1); //Set version number
				memset(buffer+1, choice, 1);

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

				memset(buffer+2, state_check, 1);
				memset(buffer+3, modifier, 1);

				printf("The server IP address: %s\n", inet_ntoa(to_server.sin_addr));
				printf("The port number: %u\n", ntohs(to_server.sin_port));
				printf("Sending protocal: %u %u %u %u\n", buffer[0], buffer[1], buffer[2],buffer[3]);

				int check = sendto(sd, buffer, DATAGRAM_SIZE, 0,(struct sockaddr*)&to_server,sizeof(to_server));
				printf("Sending size: %d\n",check);
				
				if(check < 4)
				{
					printf("Failed to write.\n");
					free(buffer);
					return 0;
				}

			} else 
			{

				memset(buffer, VERSION, 1); //Set version number
				memset(buffer+1, 0, 1);

				
				if(checkwin(board) != -1 && checkwin(board) != win_check)
				{
					state_check = 2;
					printf("ERROR: results from both users are not same.\n");
				}

				if(state_check == 2)
				{

					memset(buffer+2, state_check, 1);

					int check = sendto(sd, buffer, DATAGRAM_SIZE, 0,(struct sockaddr*)&to_server,address_length);
					printf("%d",check);
					if(check < 3)
					{
					
						printf("Failed to write.\n");
						free(buffer);
						return 0;
					}
				}
			}

		}else
		{
			printf("Invalid move ");
			if(player == player_number)
			{
				player--;
				getchar();
			} else 
			{
				memset(buffer, VERSION, 1); //Set version number
        			state_check = 2;
				memset(buffer+2, state_check, 1);
				
				if(sendto(sd, buffer, DATAGRAM_SIZE, 0,(struct sockaddr*)&to_server,sizeof(to_server)) < 4)
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

	}while (i ==  -1); // -1 means no one won
    
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
