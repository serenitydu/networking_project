/*Test*/
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
int main(int argc, char *argv[])
{
    uint8_t *recv_buffer = NULL;
	uint8_t *send_buffer = NULL;

    uint8_t version = 0;	//Fifth byte
	uint8_t move = 0;	//Sixth byte
    uint8_t error = 0;	//Fifth byte
	uint8_t e_sub = 0;	//Sixth byte
    uint8_t command = 0;	//Fifth byte
	uint8_t game_num = 0;	//Sixth byte
    socklen_t address_length;
    char *ip_addr = argv[1];
	char *port = argv[2];

    struct sockaddr_in to_server;
    address_length = sizeof(to_server);

    printf("The server IP address: %s\n",ip_addr);
	printf("The port number: %s\n",port);
	/* assigning values for sockaddr_in structure. */
	to_server.sin_family = AF_INET;
	to_server.sin_port = htons(atoi(port));
	to_server.sin_addr.s_addr = inet_addr(ip_addr);

    recv_buffer = (uint8_t*)malloc(6 * sizeof(uint8_t));
	send_buffer = (uint8_t*)malloc(6 * sizeof(uint8_t));
    while(1){
        //get command
        printf("Version: \n"); 
        scanf("%"SCNu8, &version);
        printf("move: \n"); 
        scanf("%"SCNu8, &move);
        printf("error: \n"); 
        scanf("%"SCNu8, &error);
        printf("e_sub: \n"); 
        scanf("%"SCNu8, &e_sub);
        printf("command: \n"); 
        scanf("%"SCNu8, &command);
        printf("game_num: \n"); 
        scanf("%"SCNu8, &game_num);

        memset(*buffer, version, 1);
    }
    
}