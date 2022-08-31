
// Gaurav Madkaikar
// 19CS30018
// Networks Lab Assignment 2) b) - DNS UDP Client

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 8675       // The port where clients will be connecting to
#define MAXBUFFSIZE 100 // Maximum buffer size

int main(int argc, char *argv[])
{
    int sock_client;              // Socket descriptor for the client
    struct sockaddr_in serv_addr; // Server's address information

    printf("UDP Client Side:\n");

    // Creating socket file descriptor
    sock_client = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_client < 0)
    {
        perror("\nSocket Creation Error");
        exit(EXIT_FAILURE);
    }
    printf("[+] Socket created successfully!\n");

    memset(&serv_addr, 0, sizeof(serv_addr));

    // Server information
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    // Set response time
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    // Set file descriptors
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sock_client, &readfds);

    int bytesSent;
    char dnsname[MAXBUFFSIZE];

    // DNS Prompt from the client
    printf("Enter the required DNS name: ");
    scanf("%s", dnsname);

    // Send the DNS name to the server
    bytesSent = sendto(sock_client, dnsname, strlen(dnsname), 0, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));

    // Using select statement to select ready descriptor
    select(sock_client + 1, &readfds, NULL, NULL, &tv);
    if (FD_ISSET(sock_client, &readfds))
    {
        int sizeAddr, serv_len = sizeof(serv_addr), tmpLen;
        char IPAddr[MAXBUFFSIZE];
        tmpLen = recvfrom(sock_client, &sizeAddr, sizeof(int), 0, (struct sockaddr *)&serv_addr, &serv_len);
        printf("\nCount of addresses: %d\n",sizeAddr);

        // Receive each IP Address
        printf("\nIP Addresses: \n");
        for (int i = 0; i < sizeAddr; i++)
        {
            tmpLen = recvfrom(sock_client, IPAddr, MAXBUFFSIZE, 0, (struct sockaddr *)&serv_addr, &serv_len);
            if (!strcmp(IPAddr, "0.0.0.0"))
            {
                printf("\nNo IP Address was found!\n");
                break;
            }
            printf("%s\n", IPAddr);
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
