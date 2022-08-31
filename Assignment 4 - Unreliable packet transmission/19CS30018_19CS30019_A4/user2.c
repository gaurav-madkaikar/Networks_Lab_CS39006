// Group 4
// Gaurav Madkaikar - 19CS30018
// Girish Kumar - 19CS30019

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "rsocket.h"

#define MSG_SIZE 100
#define ROLL_NO 30018

int main(int argc, char **argv)
{
    char buffer[MSG_SIZE];
    int maxSize = MSG_SIZE, bytes, CNT = 0;
    struct sockaddr_in addr_u1, addr_u2;
    socklen_t addr_len;

    printf("---------------------- USER 2 ----------------------\n");

    // Creating socket file descriptor
    int sockfd = r_socket(AF_INET, SOCK_MRP, 0);
    if (sockfd < 0)
    {
        perror("Socket creation failed!");
        exit(EXIT_FAILURE);
    }
    printf("[+] Socket created successfully\n");

    memset(&addr_u1, 0, sizeof(addr_u1));
    memset(&addr_u2, 0, sizeof(addr_u2));
    addr_u2.sin_family = AF_INET;
    addr_u2.sin_port = htons(5000 + (2 * ROLL_NO) + 1);
    addr_u2.sin_addr.s_addr = INADDR_ANY;

    // Bind to local port
    int flag = r_bind(sockfd, (const struct sockaddr *)&addr_u2, sizeof(addr_u2));
    if (flag < 0)
    {
        perror("Bind failed!");
        r_close(sockfd);
        exit(EXIT_FAILURE);
    }
    addr_len = sizeof(addr_u1);
    
    for(int i=0;i<maxSize;i++)
    {
        bytes = r_recvfrom(sockfd, buffer, maxSize, 0, (struct sockaddr *)&addr_u1, &addr_len);
        printf("%s\n", buffer);
    }
    printf("\n");
    // Close the socket
    r_close(sockfd);

    return 0;
}
