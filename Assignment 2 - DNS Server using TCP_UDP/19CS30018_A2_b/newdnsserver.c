
// Gaurav Madkaikar
// 19CS30018
// Networks Lab Assignment 2) b) - Server for both UDP and TCP clients

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>

#define MAX(a, b) ((a > b) ? a : b)
#define BACKLOG 5
#define PORT 8675       // The port client will be connecting to
#define MAXBUFFSIZE 100 // Maximum buffer size

// Start of main program
int main()
{
    // Socket descriptors
    int TCP_fd, UDP_fd;
    char buffer[MAXBUFFSIZE];
    socklen_t len;
    fd_set readfs;
    struct sockaddr_in client_addr, serv_addr;

    printf("Multi-Client Server Side:\n");

    // ++++++++++++++++++++++++++++++++ TCP Socket ++++++++++++++++++++++++++++++++
    // Create TCP Socket
    TCP_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (TCP_fd < 0)
    {
        printf("Socket creation failed!\n");
        exit(EXIT_FAILURE);
    }

    // Server Information
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);

    // Bind to the local address
    if ((bind(TCP_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
    {
        printf("TCP Socket failed to bind!\n");
        exit(EXIT_FAILURE);
    }
    // Listen to the incoming connections
    listen(TCP_fd, BACKLOG);

    // ++++++++++++++++++++++++++++++++ UDP Socket ++++++++++++++++++++++++++++++++
    // Create UDP Socket
    UDP_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (bind(UDP_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("UDP Socket failed to bind!\n");
        exit(EXIT_FAILURE);
    }

    // Set the descriptors to zero initially
    FD_ZERO(&readfs);

    // Get maximum of two descriptors to set the limit
    int maxLen = MAX(TCP_fd, UDP_fd) + 1;
    while (1)
    {
        // Set the TCP and UDP descriptors
        FD_SET(TCP_fd, &readfs);
        FD_SET(UDP_fd, &readfs);

        // Using select statement to select ready descriptor
        select(maxLen, &readfs, NULL, NULL, NULL);

        // ++++++++++++++++++++++++++ TCP part ++++++++++++++++++++++++++
        if (FD_ISSET(TCP_fd, &readfs))
        {
            printf("\n+++++++TCP Part Running...\n");
            len = sizeof(client_addr);
            // Create a new socket for data transfer
            int new_fd = accept(TCP_fd, (struct sockaddr *)&client_addr, &len);

            // Create child process using fork
            if (fork() == 0)
            {
                // Child process
                close(TCP_fd); // Closing the original TCP_fd in child process

                int buff_size;
                char buffer[MAXBUFFSIZE], strAddr[MAXBUFFSIZE];
                char *defAddr = "0.0.0.0";

                socklen_t cli_size = sizeof(client_addr);
                struct hostent *ipAdd;
                int size_addr;
                buff_size = recv(new_fd, buffer, sizeof(buffer), 0);
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
                    send(new_fd, &size_addr, sizeof(int), 0);
                    send(new_fd, defAddr, strlen(defAddr), 0);
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
                    send(new_fd, &size_addr, sizeof(int), 0);

                    for (int i = 0; i < size_addr; i++)
                    {
                        strcpy(strAddr, inet_ntoa(*((struct in_addr *)ipAdd->h_addr_list[i])));
                        int n = (int)strlen(strAddr);
                        strAddr[n] = '\n';
                        strAddr[n + 1] = '\0';
                        send(new_fd, strAddr, n + 1, 0);
                    }
                    printf("\n[+] Requested data sent successfully!\n");
                }
                close(new_fd);
                exit(EXIT_SUCCESS);
            }
            close(new_fd);
        }

        // ++++++++++++++++++++++++++ UDP part ++++++++++++++++++++++++++
        if (FD_ISSET(UDP_fd, &readfs))
        {
            printf("\n+++++++UDP Part Running...\n");

            int buff_size;
            char buffer[MAXBUFFSIZE], strAddr[MAXBUFFSIZE];
            char *defAddr = "0.0.0.0";

            socklen_t cli_size = sizeof(client_addr);
            struct hostent *ipAdd;
            int size_addr;

            buff_size = recvfrom(UDP_fd, (char *)buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &cli_size);
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
                sendto(UDP_fd, &size_addr, sizeof(int), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
                sendto(UDP_fd, defAddr, strlen(defAddr), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
            }
            else
            {
                int cnt = 0;
                while (ipAdd->h_addr_list[cnt] != NULL)
                {
                    cnt++;
                }
                size_addr = cnt;
                sendto(UDP_fd, &size_addr, sizeof(int), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));

                for (int i = 0; i < size_addr; i++)
                {
                    strcpy(strAddr, inet_ntoa(*((struct in_addr *)ipAdd->h_addr_list[i])));
                    sendto(UDP_fd, strAddr, strlen(strAddr), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
                }
                printf("\n[+] Requested data sent successfully!\n");
            }
        }

        // ++++++++++++++++++++++++++ Timeout ++++++++++++++++++++++++++
        if (!FD_ISSET(TCP_fd, &readfs) && !FD_ISSET(UDP_fd, &readfs))
        {
            printf("Requests from client side took too long...\nTimeout after 2s\nExiting...\n");
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}