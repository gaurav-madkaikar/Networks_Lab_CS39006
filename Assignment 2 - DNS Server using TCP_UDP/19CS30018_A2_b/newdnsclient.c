// Gaurav Madkaikar
// 19CS30018
// Networks Lab Assignment 2) b) - TCP DNS Client

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdint.h>

#define MAXBUFFSIZE 100 // Maximum buffer size
#define PORT 8675       // The port client will be connecting to

int main(int argc, char const *argv[])
{
    int sock_client;              // Socket descriptor for the client
    struct sockaddr_in serv_addr; // Server's address information

    printf("TCP Client Side:\n");

    // Define a TCP socket descriptor
    if ((sock_client = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("\nSocket Creation Error");
        exit(EXIT_FAILURE);
    }

    // Server information
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(serv_addr.sin_zero), '\0', 8);

    // Set response time
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    // Set file descriptors
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sock_client, &readfds);

    // Connect to the remote server address
    if (connect(sock_client, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("\nFailed to connect to server: ");
        exit(EXIT_FAILURE);
    }
    printf("[+] Socket connected successfully!\n");

    int bytesSent;
    char dnsname[MAXBUFFSIZE];

    // DNS Prompt from the client
    printf("Enter the required DNS name: ");
    scanf("%s", dnsname);

    // Send the DNS name to the server
    bytesSent = send(sock_client, dnsname, strlen(dnsname), 0);

    // Using select statement to select ready descriptor
    select(sock_client + 1, &readfds, NULL, NULL, &tv);
    if (FD_ISSET(sock_client, &readfds))
    {
        int sizeAddr, serv_len = sizeof(serv_addr), tmpLen;
        char IPAddr[MAXBUFFSIZE];
        tmpLen = recv(sock_client, &sizeAddr, sizeof(int), 0);
        printf("\nCount of addresses: %d\n", sizeAddr);

        // Receive each IP Address
        int cnt = 0, buff_sz;
        printf("\nIP Addresses: \n");
        while ((buff_sz = recv(sock_client, IPAddr, MAXBUFFSIZE, 0)) > 0)
        {
            IPAddr[buff_sz] = '\0';
            if (!strcmp(IPAddr, "0.0.0.0"))
            {
                printf("No IP Address was found!\n");
                break;
            }
            printf("%s", IPAddr);
        }
        printf("\n[+] Response received successfully!\n");
    }
    else
    {
        printf("--------------------------\nServer is busy...\nResponse took too long...\nTimeout after 2s\nExiting...\n");
        exit(EXIT_FAILURE);
    }

    close(sock_client);
    printf("Socket Closed!\n");

    return 0;
}