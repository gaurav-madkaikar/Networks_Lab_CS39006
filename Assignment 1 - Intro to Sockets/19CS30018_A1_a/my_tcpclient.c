
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

    my_tcpclient.c - Stream socket client
*/

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdint.h>

#define MAXBUFFSIZE 100 // Maximum buffer size
#define FILENAMELEN 100 // Maximum possible length of the input filename
#define BUFFSENTSIZE 80 // Amount of buffer sent in one call (MAX = 100 bytes)
#define PORT 8675       // The port client will be connecting to

int main(int argc, char const *argv[])
{
    char filename[FILENAMELEN];

    // Take the filename as a cmd-line argument
    strcpy(filename, argv[1]);

    int sock_client;              // Socket descriptor for the client
    struct sockaddr_in serv_addr; // Server's address information

    printf("Client Side:\n");

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

    // Connect to the remote server address
    if (connect(sock_client, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("\nConnection Failed");
        exit(EXIT_FAILURE);
    }
    printf("[+] Socket connected successfully!\n");

    // Open the file
    int fd;
    fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        perror("File not found");
        exit(EXIT_FAILURE);
    }

    // Reading from the input file
    char buffer[MAXBUFFSIZE + 1];
    int buff_size;

    while ((buff_size = read(fd, buffer, BUFFSENTSIZE)) > 0)
    {
        buffer[buff_size] = '\0';

        // Send the data packets to the server
        send(sock_client, buffer, strlen(buffer), 0);
    }

    // Receive the data from the server
    int cntOfWords, cntOfChars, cntOfSent;
    recv(sock_client, &cntOfChars, sizeof(int), 0);
    recv(sock_client, &cntOfWords, sizeof(int), 0);
    recv(sock_client, &cntOfSent, sizeof(int), 0);

    printf("\n[+] Details received from the server:\n");
    printf("Number of characters (except newline): %d\nNumber of words: %d\nNUmber of sentences: %d\n", cntOfChars, cntOfWords, cntOfSent);

    close(sock_client);

    return 0;
}