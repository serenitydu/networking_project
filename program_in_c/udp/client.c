/*client.c*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXLINE 80
#define SERV_PORT 8001

int main(int argc, char *argv[])
{
    struct sockaddr_in servaddr;
    int sockfd, n;
    char buf[MAXLINE];


    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr); 
    servaddr.sin_port = htons(SERV_PORT);

    while(fgets(buf, MAXLINE, stdin) != NULL){ 

        n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr*)&servaddr, sizeof(servaddr));
        if(n == -1)
            printf("sebdto err\n");

        n = recvfrom(sockfd, buf, MAXLINE, 0, NULL, 0);
        if(n == -1)
            printf("recvfrom err\n");

        write(STDOUT_FILENO, buf, n);
    }

    close(sockfd);
    return 0;
}