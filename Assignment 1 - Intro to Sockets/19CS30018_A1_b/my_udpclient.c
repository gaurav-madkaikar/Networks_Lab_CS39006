
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

    my_udpclient.c - Datagram socket client
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define FILENAMELEN 100 // Maximum possible length of the input filename
#define BUFFSENTSIZE 80 // Amount of buffer sent in one call (MAX = 100 bytes)
#define PORT 9191       // The port where clients will be connecting to
#define MAXBUFFSIZE 100 // Maximum buffer size

int main(int agrc, char *argv[])
{
    int sock_client;              // Socket descriptor for the client
    struct sockaddr_in serv_addr; // Server's address information

    // Take the filename as a cmd-line argument
    char filename[FILENAMELEN];
    strcpy(filename, argv[1]);

    printf("Client Side:\n");

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

    int fd;
    fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        perror("File not found");
        exit(EXIT_FAILURE);
    }

    // Reading from the input file
    int buff_size;
    char buffer[20];
    int numbytes;

    while ((buff_size = read(fd, buffer, BUFFSENTSIZE)) > 0)
    {
        buffer[buff_size] = '\0';

        // Send data packets to the server
        numbytes = sendto(sock_client, buffer, strlen(buffer), 0, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
    }

    printf("[+] Data sent successfully!\n");

    int serv_len = sizeof(serv_addr), noOfChars, noOfWords, noOfSent, rec_flg;
    recvfrom(sock_client, &noOfChars, sizeof(int), 0, (struct sockaddr *)&serv_addr, &serv_len);
    recvfrom(sock_client, &noOfWords, sizeof(int), 0, (struct sockaddr *)&serv_addr, &serv_len);
    recvfrom(sock_client, &noOfSent, sizeof(int), 0, (struct sockaddr *)&serv_addr, &serv_len);

    printf("\n[+] Details received from the server:\n");
    printf("Number of characters (except newline): %d\nNumber of words: %d\nNUmber of sentences: %d\n", noOfChars, noOfWords, noOfSent);

    close(sock_client);
    return 0;
}