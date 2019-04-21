/**********************************************************/
/* This program is a TCP version of tictactoe      				*/
/* The server can automatically play with multi-clients   */
/*   														               	          */
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
#include <time.h>
#include "tictactoe.h"


int main(int argc, char *argv[])
{
	/*check arguments*/
	/*If user input 2 arguments, call client function*/
	if(argc == 3)
	{
		char *ip_addr = argv[2];
		char *port = argv[1];
		client(ip_addr,port);
		printf("The server IP address: %s\n",ip_addr);
		printf("The port number: %s\n",port);

	} else 
	{
		printf("Wrong number of arguments.\n");
		printf("./tictactoeOriginal <port> <server ip> for a client.\n");
	}		
  
	return 0; 
}

//client function for a whole tictactoe game case.
int client(char *ip_addr, char *port)
{
	FILE *fp;
	char str[1000];
	char* filename = "udp_backup";

	uint8_t *recv_buffer = NULL;
	uint8_t *send_buffer = NULL;
	uint8_t game_num = 0;	//Sixth byte
	uint8_t sequence = 0;	//Seventh byte, the sequence number

	int sd;
	int connection;
	char board[ROWS][COLUMNS];
	struct sockaddr_in to_server;
	struct sockaddr_in recv;

	recv_buffer = (uint8_t*)malloc(BUFFER_SIZE * sizeof(uint8_t));
	send_buffer = (uint8_t*)malloc(BUFFER_SIZE * sizeof(uint8_t));

	/* assigning values for sockaddr_in structure. */
	to_server.sin_family = AF_INET;
	to_server.sin_port = htons(atoi(port));
	to_server.sin_addr.s_addr = inet_addr(ip_addr);

	/*create TCP socket for game*/
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        printf("Failed to create TCP socket.\n");
        exit(0); 
  } 
	printf("Created TCP socket.\n");

	/*Set connection for game*/
	if (connect(sd, (struct sockaddr*)&to_server, sizeof(to_server)) < 0) { 
			printf("Error : Connect Failed \n"); 
			exit(0); 
	} 
	

	printf("New Game Start!\n"); 
	while(1){
        // Initialize the 'game'
		memset(recv_buffer, 0, BUFFER_SIZE);
		memset(send_buffer, 0, BUFFER_SIZE);

		initSharedState(board); 
		printf("Sending message for the handshake.....\n");
		encode(&send_buffer, VERSION, NONE, NONE, NONE, NEW_GAME, NONE,sequence);

		printf("sent: %u %u %u %u %u %u %u\n", send_buffer[0], send_buffer[1], send_buffer[2], send_buffer[3], send_buffer[4], send_buffer[5], send_buffer[6]);
		write(sd,send_buffer,BUFFER_SIZE);
		sequence+=2;
		printf("Sent the request.\n");

		int recv_length = read(sd, recv_buffer, BUFFER_SIZE);
		printf("%u %u %u %u %u %u %u\n", recv_buffer[0], recv_buffer[1], recv_buffer[2], recv_buffer[3], recv_buffer[4], recv_buffer[5], recv_buffer[6]);

		if (recv_length <= 0) 
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
		/* Check the sequence again */
		if(recv_buffer[6] != sequence-1)
		{
			printf("Wrong sequence number, expect %u but %u.\n", (sequence-1),recv_buffer[6]);
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
			}
			else if(recv_buffer[3] == TRY_AGAIN){
				printf("Wrong request,try again.\n");
			}
			break;

		}
       				
		game_num = recv_buffer[5];

		printf("Game started, your game number is: %u.\n",game_num); 
		memset(send_buffer+5, game_num, 1); //Set game number
		memset(send_buffer+4, NEW_GAME, 1); //Set game number
		connection = tictactoe_client(board, sd, to_server, recv_buffer, send_buffer, &sequence, CLIENT_NUMBER); // call the 'game'
		if(connection < 0)
			close(sd);


		int connected = 1;
		while(connection<0 && connected == 1){
			/*UDP Multicast !!!*/
			struct sockaddr_in *temp = NULL;
			struct sockaddr_in *list = NULL;
			int number_of_server = 0;
			int i = 0;
			temp = client_multicast(recv_buffer,send_buffer, &recv, &number_of_server);
			list = temp;
			printf("REPORT: there is/are %d server/servers available.\n", number_of_server);
			connected = 0;
			while(i < number_of_server)
			{
				if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
					printf("Failed to create TCP socket.\n");
					exit(0); 
			  	} 

				printf("Created TCP socket.\n");

				/*Set connection for game*/
				if (connect(sd, (struct sockaddr*)(list+i), sizeof(list[i])) < 0) 
				{ 
					printf("Error : Connect Failed with server %d\n", (i+1)); 
				} else
				{

					reconnect_encode(&send_buffer, VERSION, NONE, NONE, NONE, RECONNECT, NONE, sequence,board);
					sequence = sequence + 2;
					write(sd,send_buffer,BUFFER_SIZE);
					connection = tictactoe_client(board, sd, list[i], recv_buffer, send_buffer, &sequence, SERVER_NUMBER);
					if(connection < 0)
					{
						close(sd);
						connected = 1;
						break;
					}

				}
				
				i++;
			}

			if(temp != NULL)
				free(temp);
		}
		while(connection<0 && connected == 0){
			/*Try read file to connect*/
			fp = fopen(filename, "r");
		    if (fp == NULL){
		        printf("Could not open file %s",filename);
		        break;
		    }
		    while (fgets(str, 1000, fp) != NULL){
		    	to_server.sin_addr.s_addr = inet_addr(str);
		    	fgets(str, 1000, fp);
		    	to_server.sin_port = htons(atoi(str));
		    	
					/*Set connection for game*/
					if (connect(sd, (struct sockaddr*)&to_server, sizeof(to_server)) < 0) { 
							printf("Error : Connect Failed \n"); 
							exit(0); 
					}
					reconnect_encode(&send_buffer, VERSION, NONE, NONE, NONE, RECONNECT, NONE, sequence,board);
					sequence = sequence + 2;
					write(sd,send_buffer,BUFFER_SIZE);
					connection = tictactoe_client(board, sd, to_server, recv_buffer, send_buffer, &sequence, SERVER_NUMBER);
					if(connection < 0)
						close(sd);
		    }
		        
		    fclose(fp);
		}
		
		printf("%u %u %u %u %u %u %u\n", recv_buffer[0], recv_buffer[1], recv_buffer[2], recv_buffer[3], recv_buffer[4], recv_buffer[5], recv_buffer[6]);

		break;
		
	}
	free((uint8_t*)send_buffer);
	free((uint8_t*)recv_buffer);
	close(sd);
	printf("Client close.\n");
	return 0;
}

//a function to get a list of available servers.
//return NULL if cannot find one.
struct sockaddr_in * client_multicast(uint8_t *recv_buffer,uint8_t *send_buffer, struct sockaddr_in *server_recv, int *number_of_servers)
{	
	int udp_sd, i;
	int m_length = 1;
	uint8_t temp = 1;
	struct sockaddr_in *list = NULL;
	struct sockaddr_in udp_server;
	struct timeval time_out = {4, 0};
	socklen_t address_length;
	
	uint16_t tcp_port = 0;
	
	*number_of_servers = -1;

	udp_server.sin_family = AF_INET;
	udp_server.sin_port = htons(MC_PORT);
	udp_server.sin_addr.s_addr = inet_addr(MC_GROUP);

	address_length = sizeof(udp_server);
	/*create a UDP socket for multicast*/
	if ((udp_sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
	{ 
		printf("Failed to create UDP socket.\n");
		exit(0); 
  	} 
	printf("Created UDP socket.\n");

	if(setsockopt(udp_sd, SOL_SOCKET, SO_RCVTIMEO, &time_out, sizeof(time_out)) < 0)
	{
		printf("ERROR: time out setting failed.\n");
	}


	list = malloc(sizeof(struct sockaddr_in) * 1);
	if(list == NULL)
	{
		printf("ERROR: failed to allocate a space for a multicast list.\n");
	}
	
	memset(send_buffer, 0, 2);
	memset(send_buffer, (uint8_t)VERSION, 1);
	memset(send_buffer+1, temp, 1);
	memset(recv_buffer, 0, BUFFER_SIZE);

	sendto(udp_sd, send_buffer, 2, 0,(struct sockaddr*) &udp_server, address_length);
	printf("REPORT: sending %d %d\n", send_buffer[0], send_buffer[1]);

	while(m_length > 0)
	{
		m_length = 0;
		m_length = recvfrom(udp_sd, recv_buffer, BUFFER_SIZE, 0, (struct sockaddr*)server_recv, &address_length);
		if (m_length <= 3) 
		{
			int i = 0;
			for(i = 0; i < m_length; i++)
			{
				printf("%d \n", recv_buffer[i]);
			}

		} else 
		{
			int error_check = 0;
			printf("Just received a message of the length %d.\n", m_length);
			
			/* Check the protocal version */
			if(recv_buffer[0] != VERSION)
			{
				printf("Wrong version message: %u.\n", recv_buffer[0]);
				error_check++;
			}
			/* Check the command for multicast*/
			if(recv_buffer[1] != 2)
			{
				printf("Wrong command replied from multicast server.\n");
				error_check++;
			}

			if(error_check == 0)
			{
				printf("Copying the ip address and the port number.\n");
				printf("Received: ");
				for(i = 0; i < 4; i++)
				{
					printf("%d ", recv_buffer[i]);
				}
				printf("\n");
				/*Get game port number*/
				memcpy(&tcp_port,recv_buffer+2,2);
				/*Get new ip*/
				
				
				printf("REPORT: network byte order IP address of %u\n",server_recv->sin_addr.s_addr);
				printf("REPORT: network byte order port: %d.\n", tcp_port);

				printf("REPORT: IP address of %u\n", ntohl(server_recv->sin_addr.s_addr));				
				printf("REPORT: port number: %d\n", ntohs(tcp_port));	
			
				server_recv->sin_port = 0;
				server_recv->sin_port = tcp_port;

				
				*number_of_servers+=1;
				list[*number_of_servers] = *server_recv;
				list = realloc(list, sizeof(struct sockaddr_in) * (*number_of_servers+2));
				if(list == NULL)
				{
					printf("ERROR: failed to realloc a memory space.\n");
					*number_of_servers = 0;
					return NULL;
				} else 
				{
					printf("REPORT: new server added to the list.\n");
				}
			}
		}
	}

	*number_of_servers += 1;

	return list;
}

//a function to play a tictactoe: configuring move, and sending and receiving functionalities.
int tictactoe_client(char board[ROWS][COLUMNS], int sd, struct sockaddr_in to_server,uint8_t *recv_buffer,uint8_t *send_buffer,uint8_t *sequence, int player_number)
{
	/* Modify the original tictactoe game to a networking version */


	/* Define the three bytes for transfer protocal */
	uint8_t choice;				//second byte
	uint8_t state_check;	//second byte
	uint8_t modifier = 1;	//Forth byte

	char mark;      // either an 'x' or an 'o'
	int player = player_number; // keep track of whose turn it is
	int i;  // used for keeping track of choice user makes
	int row, column, win_check;

	
	/* loop, first print the board, then ask player 'n' to make a move */
	do {
		choice = 0;
		print_board(board); // call function to print the board on the screen
		player = (player % 2) ? 1 : 2;  // Mod math to figure out who the player is
		printf("player %d's turn.\n", player);

		if(player== CLIENT_NUMBER)
		{
			/* print out player so you can pass game */
			printf("Player %d, enter a number:  ", player); 
			/* using scanf to get the choice */
			scanf("%"SCNu8, &choice);
		}else
		{
			int recv_length = 0;
			
			printf("Waiting for server playing...\n");
			recv_length = read(sd, recv_buffer, BUFFER_SIZE);

			printf("Just received the data, size: %d.\n",recv_length);
			if (recv_length <= 0) 
			{
				printf("Conenction Lost.\n");
				return -1;
			} 

			if(recv_length > 6 )
			{
				
				printf("Received: \nVERSION: %u\nPOSITION: %u\nSTATE: %u\nMODIFIER: %u\nCOMMAND: %u\nGAME NUMBER: %u\nSEQUENCE NUMBER: %u\n", recv_buffer[0], recv_buffer[1], recv_buffer[2], recv_buffer[3], recv_buffer[4], recv_buffer[5], recv_buffer[6]);
			} else 
			{
				printf("ERROR: received a message of wrong size.\n");
			}

			/* Check the protocal version */
			if(recv_buffer[0] != VERSION)
			{
				printf("ERROR: wrong version message: %u.\n", recv_buffer[0]);
				return 0;
			} 
			/* Check the sequence again */
			if(recv_buffer[6] != *sequence-1)
			{
				printf("ERROR: wrong sequence number, expect %u but %u.\n", (*sequence-1),recv_buffer[6]);
				return 0;
			}
			if(recv_buffer[4] == END_GAME)
			{
				/* break and ask for new game*/
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
				/*
				else if(recv_buffer[3] == TIME_OUT)
				{
					printf("Client game timeout.\n");
				}
				*/
				else if(recv_buffer[3] == TRY_AGAIN)
				{
					printf("Try again.\n");
				}
				return 0;

			}
			/* Check if there was an error*/	
			else if (recv_buffer[2] != GAME_IN_PROGRESS && recv_buffer[2] != GAME_COMPLETE) 
			{
				printf("ERROR: received an invalid message.\n");
				return 0;
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
						/* Client win */
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

				encode(&send_buffer, VERSION, choice, state_check, modifier, MOVE, send_buffer[5],*sequence); 
				printf("sent: %u %u %u %u %u %u %u\n", send_buffer[0], send_buffer[1], send_buffer[2], send_buffer[3], send_buffer[4], send_buffer[5], send_buffer[6]);
				int check = write(sd,send_buffer,BUFFER_SIZE);
				(*sequence)+=2;

				printf("Sending size: %d\n",check);
				
				if(check < DATAGRAM_SIZE)
				{
					printf("Failed to write.\n");
					return 0;
				}

			} else 
			{

				/*Game complete check*/
				if (recv_buffer[2] == GAME_COMPLETE)
				{
					win_check = recv_buffer[3];
					if(checkwin(board) != -1 && win_check > 0)
					{
						if((checkwin(board) == 1 && win_check == 3) || (checkwin(board) == 0 && win_check == DRAW))
						{
							encode(&send_buffer, VERSION, NONE, GAME_COMPLETE, win_check, END_GAME, send_buffer[5],*sequence); 

							printf("sent: %u %u %u %u %u %u %u\n", send_buffer[0], send_buffer[1], send_buffer[2], send_buffer[3], send_buffer[4], send_buffer[5], send_buffer[6]);
							write(sd,send_buffer,BUFFER_SIZE);
							print_board(board);
							printf("==>\aServer wins\n ");
							return 0;
						} else 
						{
								/*endgame??*/
							encode(&send_buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, END_GAME, send_buffer[5],*sequence); 
							printf("sent: %u %u %u %u %u %u %u\n", send_buffer[0], send_buffer[1], send_buffer[2], send_buffer[3], send_buffer[4], send_buffer[5], send_buffer[6]);
							write(sd,send_buffer,BUFFER_SIZE);
							return 0;
						}
					} else 
					{
						/*error: status conflict*/
						encode(&send_buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, END_GAME, send_buffer[5],*sequence); 
						printf("sent: %u %u %u %u %u %u %u\n", send_buffer[0], send_buffer[1], send_buffer[2], send_buffer[3], send_buffer[4], send_buffer[5], send_buffer[6]);
						write(sd,send_buffer,BUFFER_SIZE);
						return 0;
					}
					
				}



				memset(send_buffer+1, 0, 1);

				if(state_check == GENERAL_ERROR)
				{
					memset(send_buffer+2, state_check, 1);

					encode(&send_buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, MOVE, send_buffer[5],*sequence);
					printf("sent: %u %u %u %u %u %u %u\n", send_buffer[0], send_buffer[1], send_buffer[2], send_buffer[3], send_buffer[4], send_buffer[5], send_buffer[6]);
					int check = write(sd,send_buffer,BUFFER_SIZE);
					(*sequence)+=2;
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
				
				encode(&send_buffer, VERSION, NONE, GENERAL_ERROR, INVALID_REQUEST, MOVE, send_buffer[5],*sequence);
				printf("sent: %u %u %u %u %u %u %u\n", send_buffer[0], send_buffer[1], send_buffer[2], send_buffer[3], send_buffer[4], send_buffer[5], send_buffer[6]);
				if(write(sd,send_buffer,BUFFER_SIZE) < DATAGRAM_SIZE)
				{
					printf("Failed to write.\n");
					return 0;
				}
				(*sequence)+=2;
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
		printf("==>\aClient wins\n ");
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

//encoding function for a message.
void encode(uint8_t **buffer, uint8_t version, uint8_t position, uint8_t error, uint8_t modifier, 
uint8_t command, uint8_t game_number, uint8_t sequence)
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

//encoding function for a reconnect case.
void reconnect_encode(uint8_t **buffer, uint8_t version, uint8_t position, uint8_t error, uint8_t modifier, 
uint8_t command, uint8_t game_number, uint8_t sequence, char board[ROWS][COLUMNS])
{
	int i, j, d;
	memset(*buffer, 0, DATAGRAM_SIZE);
	memset(*buffer, version, 1);
	memset((*buffer)+1, position, 1);
	memset((*buffer)+2, error, 1);
	memset((*buffer)+3, modifier, 1);
	memset((*buffer)+4, command, 1);
	memset((*buffer)+5, game_number, 1);
	memset((*buffer)+6, sequence, 1);
	d = 7;
	for (i=0;i<3;i++)
        for (j=0;j<3;j++){
		if(board[i][j] == 'O'){
			memset((*buffer)+d, 1, 1);
		}else if(board[i][j] == 'X'){
			memset((*buffer)+d, 2, 1);
		}else{
			memset((*buffer)+d, 0, 1);
		}
            d++;
        }
}
