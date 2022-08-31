// Gaurav Madkaikar
// 19CS30018
// Networks Lab Assignment 1) b) - UDP Socket Programming

/*
    Conventions for the text within the input file:
    a) Words can comprise of alphanumeric characters
    b) Delimiters are special characters which separate words.
    c) '\n' is not considered in the total count of characters. If required they can be included in the
    overall character count by commenting line 123 in the file "my_tcpserver.c"
 ***d) Files should end with a '#' to denote end of file

    **Server handles one connection at a time and closes once the 
    client closes
    my_udpserver.c - Datagram socket server
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 9191       // The port where clients will be connecting to
#define MAXBUFFSIZE 100 // Maximum buffer size

// Check if a character is a delimiter
int delimiterCheck(char ch)
{
    if (ch == ' ' || ch == '.' || ch == ',' || ch == '\n')
        return 1;

    return 0;
}

int main(int argc, char *argv[])
{
    int sock_serv;
    struct sockaddr_in serv_addr, client_addr;

    printf("Server Side:\n");

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

    int cntOfWords = 0, cntOfChars = 0, cntOfSent = 0;
    int prevCharflg = 0, breakFlg = 0, validTrans = 0;

    socklen_t cli_size = sizeof(client_addr);
    while ((buff_size = recvfrom(sock_serv, (char *)buffer, MAXBUFFSIZE+1, 0, (struct sockaddr *)&client_addr, &cli_size)) > 0)
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
                breakFlg = 1;
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
        if (breakFlg)
            break;
    }
    if (validTrans)
        printf("\n[+] Data received successfully!\n");

    // Send the required count of characters, words, and sentences back to the client
    sendto(sock_serv, &cntOfChars, sizeof(int), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
    sendto(sock_serv, &cntOfWords, sizeof(int), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
    sendto(sock_serv, &cntOfSent, sizeof(int), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));

    printf("[+] Data sent to the client successfully!\n");

    // Close the socket
    close(sock_serv);
    return 0;
}