/**********************************************************/
/* This program is a 'pass and play' version of tictactoe */
/* Two users, player 1 and player 2, pass the game back   */
/* and forth, on a single computer                        */
/**********************************************************/

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

/* #define section, for now we will define the number of rows and columns */
#define ROWS  3
#define COLUMNS  3
#define BUFFER_SIZE 3
#define SERVER_NUMBER 1
#define CLIENT_NUMBER 2
#define VERSION 2

/* C language requires that you predefine all the routines you are writing */
int checkwin(char board[ROWS][COLUMNS]);
void print_board(char board[ROWS][COLUMNS]);
int tictactoe(char board[ROWS][COLUMNS], int player_number, int sd);
int initSharedState(char board[ROWS][COLUMNS]);
int client(char *ip_addr, char *port);
int server(char *port);

int main(int argc, char *argv[])
{

	if(argc > 2)
	{
		if(strlen(argv[2]) != 1 || (atoi(argv[2]) != 1 && atoi(argv[2]) != 2))
		{
			printf("Invalid player name.\n");
			printf("1 for a server, and 2 for a client.\n");
			return 0;
		} 
		
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

		} else if(argc == 3)
		{
			if(atoi(argv[2]) == 1)
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
			printf("./tictactoeOriginal <port> <player number> <server ip> for a client.\n");
			printf("./tictactoeOriginal <port> <player number> for a server.\n");
		}		
	} else 
	{
		printf("Wrong number of arguments.\n");
		printf("./tictactoeOriginal <port> <player number> <server ip> for a client.\n");
		printf("./tictactoeOriginal <port> <player number> for a server.\n");
	}
  
	return 0; 
}

int client(char *ip_addr, char *port)
{

	char board[ROWS][COLUMNS];

	printf("The server IP address: %s\n",ip_addr);
	printf("The port number: %s\n",port);

	int sd = 0;
	struct sockaddr_in server;

	//assigning values for sockaddr_in structure.
	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(port));
	server.sin_addr.s_addr = inet_addr(ip_addr);
    
 	sd = socket(AF_INET, SOCK_STREAM, 0);

	//socket error check.
	if(sd < 0)
	{
		printf("Failed to create a socket.\n");
		exit(1);
	}

	printf("Created a socket.\n");

    	//making connection with the server.
	if(connect(sd, (struct sockaddr *) &server, sizeof(struct sockaddr_in)) < 0)
	{

		printf("Connection failed.\n");
		
	} else
	{
		printf("Connection created with the server.\n");

		initSharedState(board); // Initialize the 'game' board
		tictactoe(board,  CLIENT_NUMBER, sd); // call the 'game' 
	}
		
	close(sd);

	return 0;
}

int server(char *port)
{

	/* These variables are used to initiate connecting.*/
	int sd;             /*socket descriptor*/
	int client_sd;   /*socket descriptor*/ 
	unsigned int addr_size = 0;
	char board[ROWS][COLUMNS];
	struct sockaddr_in server_addr;
	struct sockaddr_in from_addr;

	
	printf("The port number: %s\n",port);

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(port));
	server_addr.sin_addr.s_addr = INADDR_ANY;
    
	sd = socket(AF_INET,SOCK_STREAM,0);

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

	while(1)
    	{
        	listen(sd, 1);
        	printf("Listening...\n");

        	client_sd=accept(sd,(struct sockaddr*)&from_addr, &addr_size);

        	if(client_sd < 0)
		{
			printf("Failed to connect to the client.\n");		
		}
		else
		{
			printf("Get connection from a client.\n");
			initSharedState(board); // Initialize the 'game' board
			tictactoe(board, SERVER_NUMBER, client_sd); // call the 'game'
			close(client_sd);
		}
 
	}

	close(sd);    
	return 0;
}

int tictactoe(char board[ROWS][COLUMNS], int player_number, int sd)
{
	/* this is the meat of the game, you'll look here for how to change it up */

	char mark;      // either an 'x' or an 'o'
	int player = 2; // keep track of whose turn it is
	int i;  // used for keeping track of choice user makes
	int row, column;

	//Define the three bytes for transfer protocal
	uint8_t choice;
	uint8_t state_check;
	uint8_t *buffer;


	/* loop, first print the board, then ask player 'n' to make a move */

	do {
		choice = 0;
		print_board(board); // call function to print the board on the screen
		player = (player % 2) ? 1 : 2;  // Mod math to figure out who the player is

        	buffer = (uint8_t*) calloc(BUFFER_SIZE, sizeof(uint8_t));

		memset(buffer, 0, BUFFER_SIZE);

		if(player== player_number)
		{
			printf("Player %d, enter a number:  ", player); // print out player so you can pass game
			scanf("%"SCNu8, &choice); //using scanf to get the choice

		} 
        	else
		{
			int i;
			for(i=0; i<BUFFER_SIZE; i++)
			{
				int error = read(sd, buffer+i, 1);
				if(error < 0 )
				{
					printf("Faild to receive a data.\n");
					free(buffer);
					return 0;
				} else if (error == 0) 
				{
					printf("Conenction Lost.\n");
					free(buffer);
					return 0;
				}
			}
			

			
			if(buffer[0] != VERSION)
			{
				printf("Wrong version message: %u.\n", buffer[0]);
				free(buffer);
				return 0;
			} 
            		else if (buffer[2] == 2)
			{
				printf("There was an error.\n");
				free(buffer);
				return 0;

			} else if (buffer[2] != 0 && buffer[2] != 1) 
			{
				printf("Received an invalid message.\n");
				free(buffer);
				return 0;
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
				memset(buffer, VERSION, 1); //Set version number
				memset(buffer+1, choice, 1);
				printf("%u %u %u\n", buffer[0], buffer[1], buffer[2]);

				if(checkwin(board) != -1)
				{
                    state_check = 1;
				} else 
				{
                	state_check = 0;
					 
				}

				memset(buffer+2, state_check, 1);


				if(write(sd, buffer, 3) < 1)
				{
					printf("Failed to write.\n");
					free(buffer);
					return 0;
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
				memset(buffer, VERSION, 1); //Set version number
                		state_check = 2;
				memset(buffer+2, state_check, 1);


				if(write(sd, buffer, 3) < 1)
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
