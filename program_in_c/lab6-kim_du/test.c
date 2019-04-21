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
	if(argc == 3)
	{
		char *ip_addr = argv[2];
		char *port = argv[1];
		client(ip_addr,port);
		printf("The server IP address: %s\n",ip_addr);
		printf("The port number: %s\n",port);

	}else 
	{
		printf("Wrong number of arguments.\n");
		printf("./tictactoeOriginal <port> <server ip> for a client.\n");
	}		
  
	return 0; 
}


int client(char *ip_addr, char *port)
{

	uint8_t *recv_buffer = NULL;
	uint8_t *send_buffer = NULL;
	uint8_t *prev_buffer = NULL;
	socklen_t address_length;
	uint8_t sequence = 0;	//Seventh byte, the sequence number
	int sd = 0;
	int try = 0;
	struct sockaddr_in to_server;
	struct sockaddr_in sa_client;
	struct timeval timeout={15,0}; //Set timeout interval
	char *client_port = "10000";

	address_length = sizeof(to_server);
	/* assigning values for sockaddr_in structure. */
	to_server.sin_family = AF_INET;
	to_server.sin_port = htons(atoi(port));
	to_server.sin_addr.s_addr = inet_addr(ip_addr);
    
    sa_client.sin_family = AF_INET;
	sa_client.sin_port = htons(atoi(client_port));
	sa_client.sin_addr.s_addr = htonl(INADDR_ANY);

 	sd = socket(AF_INET,SOCK_DGRAM,0);

 	if(bind(sd,(struct sockaddr*)&sa_client,sizeof(struct sockaddr))<0)
	{
        	printf("ERROR: Bind error\n");
		close(sd);
        	return -1;
   	}
	/* socket error check. */
	if(sd < 0)
	{
		printf("Failed to create a socket.\n");
		exit(1);
	}
	/* Bind timeout with the socket*/
	setsockopt(sd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(struct timeval));
	printf("Created a socket.\n");

	recv_buffer = (uint8_t*)malloc(BUFFER_SIZE * sizeof(uint8_t));
	send_buffer = (uint8_t*)malloc(DATAGRAM_SIZE * sizeof(uint8_t));
	prev_buffer = (uint8_t*)malloc(DATAGRAM_SIZE * sizeof(uint8_t));

	while(1)
	{
		uint8_t a,b,c,d,e,f,g;
		printf("Enter your input.\n");
		scanf("%hhu %hhu %hhu %hhu %hhu %hhu %hhu", &a, &b, &c, &d, &e, &f, &g);
		printf("Version: %" PRIu8 "\n", a);
		printf("Position: %" PRIu8 "\n", b);
		printf("Status: %" PRIu8 "\n", c);
		printf("Description: %" PRIu8 "\n", d);
		printf("Command: %" PRIu8 "\n", e);
		printf("Game Number: %" PRIu8 "\n", f);
		printf("Sequence number: %" PRIu8 ".\n", g);

		memset(recv_buffer, 0, BUFFER_SIZE);
		memset(send_buffer, 0, DATAGRAM_SIZE);

	
		encode(&send_buffer, a, b, c, d, e, f, g);
		encode(&prev_buffer, a, b, c, d, e, f, g);
		sendto(sd, send_buffer, DATAGRAM_SIZE, 0,(struct sockaddr*)&to_server,sizeof(to_server));
		
		int recv_length = recvfrom(sd, recv_buffer, DATAGRAM_SIZE, 0, (struct sockaddr*) &to_server, &address_length);

		while((recv_length < 0 || recv_buffer[6] != (sequence-1)) && try < SHUT_DOWN)
		{
			printf("Resend the request, %d try.\n",try+1);
			printf("Resent: %u %u %u %u %u %u %u\n", send_buffer[0], send_buffer[1], send_buffer[2], send_buffer[3], send_buffer[4], send_buffer[5], send_buffer[6]);
			sendto(sd, prev_buffer, DATAGRAM_SIZE, 0,(struct sockaddr*)&to_server,sizeof(to_server));
			memset(recv_buffer, 0, BUFFER_SIZE);
			try++;
			recv_length = recvfrom(sd, recv_buffer, DATAGRAM_SIZE, 0, (struct sockaddr*) &to_server, &address_length);
			printf("Receive: %u %u %u %u %u %u %u\n", recv_buffer[0], recv_buffer[1], recv_buffer[2], recv_buffer[3], recv_buffer[4], recv_buffer[5], recv_buffer[6]);
		}
		try = 0;

		printf("Received a data.\n");
		printf("Version: %" PRIu8 "\n", recv_buffer[0]);
		printf("Position: %" PRIu8 "\n", recv_buffer[1]);
		printf("Status: %" PRIu8 "\n", recv_buffer[2]);
		printf("Description: %" PRIu8 "\n", recv_buffer[3]);
		printf("Command: %" PRIu8 "\n", recv_buffer[4]);
		printf("Game Number: %" PRIu8 "\n", recv_buffer[5]);
		printf("Sequence number: %" PRIu8 ".\n", recv_buffer[6]);

		if(recv_length < 0 )
		{
			printf("Faild to receive a data.\n");
		} 
		else if (recv_length == 0) 
		{
			printf("Conenction Lost.\n");
		} 	
		
	
	}

	close(sd);

	return 0;
}

 
void encode(uint8_t **buffer, uint8_t version, uint8_t position, uint8_t error, uint8_t modifier, uint8_t command, uint8_t game_number, uint8_t sequence)
{
	memset(*buffer, version, 1);
	memset((*buffer)+1, position, 1);
	memset((*buffer)+2, error, 1);
	memset((*buffer)+3, modifier, 1);
	memset((*buffer)+4, command, 1);
	memset((*buffer)+5, game_number, 1);
	memset((*buffer)+6, sequence, 1);	
}

