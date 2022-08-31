
// Gaurav Madkaikar
// 19CS30018
// Networks Lab Assignment 1) a) - TCP Socket Programming

/*
    Conventions for the text within the input file:
    a) Words can comprise of alphanumeric characters
    b) Delimiters are special characters which separate words.
    c) '\n' is not considered in the total count of characters. If required they can be included in the
    overall character count by commenting line 123 in the file "my_tcpserver.c"
 ***d) Files should end with a '#' to denote end of file

    my_tcpserver.c - Stream socket server
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdint.h>

#define MAXBUFFSIZE 100 // Maximum buffer size
#define PORT 8675       // The port where clients will be connecting to
#define PENDING 5       // The number of pending connections that can be accomodated

// Check if a character is a delimiter
int delimiterCheck(char ch)
{
    if (ch == ' ' || ch == '.' || ch == ',' || ch == '\n')
        return 1;

    return 0;
}

int main(int argc, char const *argv[])
{
    int sock_serv;                  // Socket descriptor for the server
    struct sockaddr_in serv_addr;   // Server's address information
    struct sockaddr_in client_addr; // Connector’s address information

    printf("Server Side:\n");

    // Define a TCP socket descriptor
    if ((sock_serv = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("\nSocket Creation Error");
        exit(EXIT_FAILURE);
    }
    printf("[+] Socket created successfully!\n");

    int opt = 1;
    // Reuse the port if not available
    if (setsockopt(sock_serv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) == -1)
    {
        perror("\nPort cannot be reused");
        exit(EXIT_FAILURE);
    }

    // Server information
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(serv_addr.sin_zero), '\0', 8);

    // Bind the socket to the server address
    if (bind(sock_serv, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("Socket cannot be bound");
        exit(EXIT_FAILURE);
    }
    printf("[+] Server is running.....\n");

    // Listen to incoming connections and handle them in a queue
    if (listen(sock_serv, PENDING) == -1)
    {
        perror("Error in listening to connections");
        exit(EXIT_FAILURE);
    }

    int new_fd; // Socket descriptor for new connections

    // Accept incoming connections one at a time
    while (1)
    {
        int saddr_size = sizeof(struct sockaddr_in);
        if ((new_fd = accept(sock_serv, (struct sockaddr *)&client_addr, &saddr_size)) == -1)
        {
            perror("Cannot accept connection");
            exit(EXIT_FAILURE);
        }

        // Store the received buffer
        char buffer[MAXBUFFSIZE + 1];
        int buff_size;

        // Variables for storing required details
        int cntOfWords = 0, cntOfChars = 0, cntOfSent = 0;
        int prevCharflg = 0;
        int breakflg = 0;
        int validTrans = 0;

        // Receive data in chunks from the client
        while ((buff_size = recv(new_fd, buffer, sizeof(buffer), 0)) > 0)
        {
            validTrans = 1;
            buffer[buff_size] = '\0';

            // Counting number of characters, words and sentences
            int trav = 0, prev = 0;
            while (buffer[trav] != '\0')
            {
                // Break if we have read the entire file
                // '#' denotes EOF
                if (buffer[trav] == '#')
                {
                    breakflg = 1;
                    break;
                }
                // Count characters except newline
                if (buffer[trav] != '\n')
                    cntOfChars++;

                // Count words and sentences
                if (delimiterCheck(buffer[trav]))
                {
                    if (trav == 0)
                        cntOfWords++;

                    if (buffer[trav] == '.')
                        cntOfSent++;

                    if (trav > 0 && !delimiterCheck(buffer[prev]))
                        cntOfWords++;
                }
                else if (trav == buff_size - 1)
                {
                    prevCharflg = 1;
                }
                prev = trav;
                trav++;
            }
            // Break once no data is left to be read
            if (breakflg)
                break;
        }
        if (validTrans)
            printf("[+] Data received successfully!\n");

        // Send the details regarding the input file back to the client
        send(new_fd, &cntOfChars, sizeof(int), 0);
        send(new_fd, &cntOfWords, sizeof(int), 0);
        send(new_fd, &cntOfSent, sizeof(int), 0);

        printf("[+] Data sent to the client successfully!\n");

        // Close the socket
        close(new_fd);
    }
    close(sock_serv);
    return 0;
}