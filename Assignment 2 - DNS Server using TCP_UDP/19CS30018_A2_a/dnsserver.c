
// Gaurav Madkaikar
// 19CS30018
// Networks Lab Assignment 2) a) - UDP DNS Server

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#define PORT 9191       // The port where clients will be connecting to
#define MAXBUFFSIZE 100 // Maximum buffer size

int main(int argc, char *argv[])
{
    int sock_serv;
    struct sockaddr_in serv_addr, client_addr;

    printf("DNS Server Side:\n");

    // Creating socket file descriptor
    sock_serv = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_serv < 0)
    {
        perror("\nSocket Creation Error");
        exit(EXIT_FAILURE);
    }
    printf("[+] Socket created successfully!\n");

    memset(&serv_addr, 0, sizeof(serv_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    // Server Information
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    // Bind the socket with the server address
    if (bind(sock_serv, (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Socket cannot be bound");
        exit(EXIT_FAILURE);
    }

    printf("[+] Server is running.....\n");

    int buff_size;
    char buffer[MAXBUFFSIZE + 1];

    char *defAddr = "0.0.0.0";
    char strAddr[MAXBUFFSIZE];

    socklen_t cli_size = sizeof(client_addr);
    while (1)
    {
        printf("\nNew Request:\n");
        struct hostent *ipAdd;
        int size_addr;

        buff_size = recvfrom(sock_serv, (char *)buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &cli_size);
        buffer[buff_size] = '\0';
        if (buff_size <= 0)
        {
            printf("[-] Failed to receive a valid DNS name!\n");
            exit(EXIT_FAILURE);
        }
        printf("\nDNS name received from the client: %s\n", buffer);

        ipAdd = gethostbyname(buffer);
        if (ipAdd == NULL)
        {
            size_addr = 1;
            sendto(sock_serv, &size_addr, sizeof(int), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
            sendto(sock_serv, defAddr, strlen(defAddr), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
            // Close the socket
            close(sock_serv);
            printf("\n[+] Socket Closed!\n[+] Exiting...\n");
            exit(EXIT_SUCCESS);
        }
        else
        {
            int cnt = 0;
            while (ipAdd->h_addr_list[cnt] != NULL)
            {
                cnt++;
            }
            size_addr = cnt;
            sendto(sock_serv, &size_addr, sizeof(int), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));

            for (int i = 0; i < size_addr; i++)
            {
                strcpy(strAddr, inet_ntoa(*((struct in_addr *)ipAdd->h_addr_list[i])));
                sendto(sock_serv, strAddr, strlen(strAddr), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
            }
            printf("\n[+] Requested data sent successfully!\n");
        }
    }

    // Close the socket
    close(sock_serv);
    printf("\n[+] Socket Closed!\n[+] Exiting...\n");
    return 0;
}