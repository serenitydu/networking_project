/*
	CSE 5462 
	Lab 2 Assignment
    Server
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

#define PORT_NUM 9999
#define MAXPORT 5
#define BUFFER_SIZE 1000
#define FILE_NAME_SIZE 20
/*This is a FTP server*/

int main(int argc, char const *argv[]){
    /* These variables are used to initiate connecting.*/
    int sd;             /*socket descriptor*/
    int connected_sd;   /*socket descriptor*/
    //int rc;             /*return code from recevfrom*/

    struct sockaddr_in server_addr;
    struct sockaddr_in from_addr;
    unsigned int socket_size;
    //char* buffer;
    char *ptr;

    /* These variables are used to receive data from client.*/
    
    unsigned long file_size, left_size, length, recv_size;
    char buffer[BUFFER_SIZE];
    char file_name[FILE_NAME_SIZE + 1];
	char directory[FILE_NAME_SIZE + 7] = "recvd/";
    FILE *fp;

    sd = socket(AF_INET,SOCK_STREAM,0);
    /*if socket failed to initiate, report the error*/
    if(sd == -1){
		printf("ERROR: Socket failed:%d",errno);
		return -1;
	}
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[1]));
    server_addr.sin_addr.s_addr = INADDR_ANY;
    /*Set other attributes to 0*/
    bzero(&(server_addr.sin_zero),8);

    /*bind the socket with ip address and port*/
    if(bind(sd,(struct sockaddr*)&server_addr,sizeof(struct sockaddr))<0){
		printf("ERROR: Bind error\n");
		return -1;
	}

    listen(sd, MAXPORT);
    printf("Listening...\n");

    while(1){
        /*Waiting for connection*/
        connected_sd=accept(sd,(struct sockaddr*)&from_addr, &socket_size);
        if(connected_sd < 0){
            printf("ERROR: Connection failed\n");
        }else{
            printf("Get connection from server\n");
        } 
        
        bzero(buffer, sizeof(buffer));
        length = read(connected_sd, buffer, 4);
        if (length < 0){
            printf("ERROR: Server Recieve Data Failed!\n");
            close(connected_sd);
            continue;
        }
        /* Get the length of the file*/
        if(length == 4){
            file_size = ntohl(strtoul(buffer, &ptr, 10));
            printf("Server receives the file size: %u\n",file_size);
        }else{
            printf("ERROR: Server Recieve File Size Failed!\n");
            close(connected_sd);
            continue;
        }

        /* Read file name and create the file in server folder*/
       	memset(buffer, '\0', 21);
	    memset(file_name, '\0', 21);
        length = read(connected_sd, buffer, 20);
        if(length == 20){
            strncpy(file_name, buffer, strlen(buffer));
		    strcat(directory, file_name);
            if ((fp = fopen(directory,"wb")) == NULL){
                printf("File:\t%s Create Failed!\n", file_name);
            } else {printf("File is successfully created.\n");}
        }else{
            printf("Server Recieve File Name Failed!\n");
            close(connected_sd);
            continue;
        }
        
        /* Save file*/
        left_size = file_size;
        while(left_size >= 0){
            bzero(buffer, sizeof(buffer));
            length = read(connected_sd, buffer, BUFFER_SIZE);
		    printf("%u\n.", length);
            if(length <0){
                printf("ERROR: Server Recieve File Contant Failed!\n");
                close(connected_sd);
                break;
            } else {
		        printf("Succesffully read the data.\n");
	        }
            left_size = left_size - length;
            
            if(fwrite(buffer,sizeof(char),length,fp) != length){
                printf("ERROR: Server Write File Contant Error!\n");
		        break;
            }
        }
	    fclose(fp);
        recv_size = file_size - left_size;
        /*Send the received file size to the client*/
        write(connected_sd, &recv_size, 4);
        printf("Server Store File Successfully!!!\n");
        close(connected_sd);
            
    }

    return 0;
}