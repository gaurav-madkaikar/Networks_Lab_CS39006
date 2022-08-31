// Group 4
// Gaurav Madkaikar - 19CS30018
// Girish Kumar - 19CS30019

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "rsocket.h"

#define MSG_SIZE 100
#define ROLL_NO 30018

int main(int argc, char **argv)
{
    char buff[MSG_SIZE];
    struct sockaddr_in addr_u2, addr_u1;
    socklen_t len;

    printf("---------------------- USER 1 ----------------------\n");

    // Creating socket file descriptor
    int sockfd = r_socket(AF_INET, SOCK_MRP, 0);
    if (sockfd < 0)
    {
        perror("Socket creation failed!");
        exit(1);
    }
    printf("[+] Socket created successfully\n");

    // Local port
    memset(&addr_u1, 0, sizeof(addr_u1));
    addr_u1.sin_family = AF_INET;
    addr_u1.sin_port = htons(5000 + (2 * ROLL_NO));
    addr_u1.sin_addr.s_addr = INADDR_ANY;

    // Port where data is to be sent
    memset(&addr_u2, 0, sizeof(addr_u2));
    addr_u2.sin_family = AF_INET;
    addr_u2.sin_port = htons(5000 + (2 * ROLL_NO) + 1);
    addr_u2.sin_addr.s_addr = INADDR_ANY;

    // Bind to the local port
    int bindFlg = r_bind(sockfd, (const struct sockaddr *)&addr_u1, sizeof(addr_u1));
    if (bindFlg < 0)
    {
        perror("Bind failed!");
        r_close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Take input string
    printf("Enter input string:  ");
    scanf("%[^\n]", buff);
    len = strlen(buff);

    // Send each character as a message
    for (int i = 0; i < (int)len; i++)
    {
        int addrlen = sizeof(addr_u2);
        char buffer[] = {buff[i], '\0'};
        r_sendto(sockfd, buffer, 1, 0, (const struct sockaddr *)&addr_u2, addrlen);
    }

    // Close the socket
    r_close(sockfd);

    return 0;
}