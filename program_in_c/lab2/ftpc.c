/*
	CSE 5462 
	Lab 2 Assignment
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main (int argc, char **argv)
{
	int sd = 0;
	int error = 0;
	unsigned long file_size = 0;
	void *buffer = NULL;
	struct sockaddr_in server;
	FILE *file = NULL;

	//checking a number of arguments.
	if (argc != 4)
	{
		printf("There should be 3 arguments.\n");
		printf("ftpc <remote-IP> <remote-port> <local-file-to-transfer>.\n");
		exit(1);
	} 
	
	//allocating a memory space for the buffer.
	if((buffer = malloc(sizeof(char) * 1000)) == NULL)
	{
		printf("Failed to allocate a memory space for a buffer.\n");
		exit(1);
	}
	
	//checking the title.
	if(strlen(argv[3]) > 20)
	{
		printf("Title should be smaller than or equal to 20 characters.\n.");
		free(buffer);
		exit(1);
	}

	//opening the file to read.
	file = fopen(argv[3], "rb");
	if(file == NULL)
	{
		printf("Failed to open the file.\n");
		free(buffer);
		exit(1);
	}

	//configuring a size of the file to send.
	if (fseek(file, 0, SEEK_END) != 0)
	{
		printf("Failed to measure the size of the file.\n");
		fclose(buffer);
		free(buffer);		
		exit(1);	
	}

	file_size = ftell(file);


	if(file_size == -1L)
	{
		printf("Failed to measure the size of the file.\n");
		fclose(buffer);
		free(buffer);		
		exit(1);
	} else 
	{
		printf("Successfully measured the size of the file: %lu\n", file_size);
	}
	rewind(file);
	
	//assigning values for sockaddr_in structure.
	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(argv[2]));
	server.sin_addr.s_addr = inet_addr(argv[1]);

	//creating a socket.
	sd = socket(AF_INET, SOCK_STREAM, 0);

	//socket error check.
	if(sd < 0)
	{
		printf("Failed to create a socket.\n");
		exit(1);
	}

	printf("Created a socket.\n");

	//making connection with the server.
	if(connect(sd, (struct sockaddr *) &server, sizeof(struct sockaddr_in)) <0)
	{
		printf("Connection failed.\n");
	}

	printf("Connection created with the server.\n");
	
	memmove(buffer, &file_size, 4);

	if(write(sd, buffer, 4) != -1)
	{
		memset(buffer, ' ', 20);
		memmove(buffer, argv[3], strlen(argv[3]));
		if(write(sd, buffer, 20) != -1)
		{
			while(file_size > 0)
			{
				int scanned_size = 0;
				scanned_size = fread(buffer, sizeof(char), 1000, file);				
				memset(buffer+scanned_size, 0, 1000-scanned_size);
				file_size = file_size - scanned_size;
				if(write(sd, buffer, scanned_size) == -1)
				{
					error = 1;
					break;
				} 
			}
		} else
		{
			error = 1;
		}
		
	} else 
	{	
		error = 1;
	}

	if(error)
	{
		printf("Failed to send a data to the server.\n");
	} else 
	{
		memset(buffer, 0, 4);
		int received = 0;
		received = read(sd, buffer, 4);
		if(received < 4)
		{
			printf("There has been an error at receiving a data from the server.\n");
		} else 
		{
			if(memcmp(buffer, &file_size, 4) == 0)
			{
				printf("Failed to fully send the file.\n");
			} else 
			{
				printf("Successfully sent the file.\n");
			}
		}
	}

	printf("Closing the connection with ther server.\n");

	close(sd);
	fclose(file);	
	free(buffer);

	return 0;
}