// Group 4
// Gaurav Madkaikar (19CS30018)
// Girish Kumar (19CS30019)
// FTP Server

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
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <string.h>
#include <stdint.h>

#define PORT 21000
#define BACKLOG 10
#define MAXSIZE 300
#define MAXBUFFSIZE 50
#define ERR_SIZE 4
#define CHUNKSIZE 15

typedef struct user
{
    char name[20];
    char password[20];

} user;
// Store user information
user info[2];

void readUserInfo(FILE *fptr)
{
    char ch;
    int user_id = 0, chseen = 0, i = 0, spseen = 0;
    while ((ch = fgetc(fptr)) != EOF)
    {
        if (ch == '\n')
        {
            info[user_id].password[i] = '\0';
            user_id++;
            chseen = 0;
            i = 0;
            spseen = 0;
        }
        else if (ch == ' ')
        {
            if (!spseen)
                info[user_id].name[i] = '\0', spseen = 1;
            if (chseen)
                i = 0, spseen = 1;
            continue;
        }
        else
        {
            if (!spseen)
            {
                info[user_id].name[i++] = ch, chseen = 1;
            }
            else
            {
                info[user_id].password[i++] = ch;
            }
        }
    }
}

int findCommandName(char command[MAXSIZE], char cmdName[MAXSIZE])
{
    int trav = 0, fsp = 0, i = 0, j = 0;
    while (command[trav] != '\0')
    {
        if (((command[trav] == '\n') || (command[trav] == ' ')) && (!fsp))
        {
            cmdName[i] = '\0';
            fsp = 1;
            break;
        }
        else
        {
            cmdName[i++] = command[trav++];
        }
    }
    if (!fsp)
    {
        cmdName[trav] = '\0';
    }
    return trav;
}

int main(int argc, char *argv[])
{
    // Socket descriptors
    int server_fd;
    socklen_t len;
    fd_set readfs;
    struct sockaddr_in client_addr, serv_addr;

    // ++++++++++++++++++++++++++++++++ TCP Socket ++++++++++++++++++++++++++++++++
    // Create TCP Socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
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
    if ((bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
    {
        printf("TCP Socket failed to bind!\n");
        exit(EXIT_FAILURE);
    }
    // Listen to the incoming connections
    if (listen(server_fd, BACKLOG) < 0)
    {
        close(server_fd);
        perror("Error in Listen()");
        exit(EXIT_FAILURE);
    }
    int newsockfd;
    printf("------------- Waiting for client connections! -------------\n\n");
    while (1)
    {
        len = sizeof(client_addr);
        // Create a new socket for data transfer
        newsockfd = accept(server_fd, (struct sockaddr *)&client_addr, &len);
        if (newsockfd < 0)
        {
            printf("ERR0R: Cannot connect to server!\n");
        }
        // Create child process using fork
        else
        {
            // Child process
            if (fork() == 0)
            {
                // Closing the original server_fd in child process
                close(server_fd);

                // Runnable code .... (START)

                // Authenticated users
                FILE *fptr;
                fptr = fopen("user.txt", "r");
                if (fptr == NULL)
                {
                    perror("Cannot open user.txt");
                    exit(EXIT_FAILURE);
                }
                // Read user information
                readUserInfo(fptr);

                fclose(fptr);

                printf("--------------- FTP Server Side ---------------\n\n");

                // Required data variables
                // Flag to indicate the first & second commands received
                int fcmd = 1, scmd = 1;
                // Error Code of each command
                char err_code[ERR_SIZE];

                // Matched user (Initially no user is logged in)
                int matched_user = -1;
                int bytes, cmdSize = 0, cnt = 0;

                // Receive first command which is port
                char buffer[MAXSIZE], command[MAXSIZE], local_file[MAXSIZE], remote_file[MAXSIZE];
                char cmdName[MAXSIZE], args[MAXSIZE], fnames[MAXSIZE][MAXSIZE];

                while (1)
                {
                    memset(command, '\0', sizeof(command));
                    memset(cmdName, '\0', sizeof(cmdName));
                    // Receive command from the server
                    cmdSize = recv(newsockfd, command, MAXSIZE, 0);
                    command[cmdSize] = '\0';
                    // Find command name
                    int trav = findCommandName(command, cmdName);

                    if (fcmd)
                    {
                        if (strcmp(cmdName, "user") == 0)
                        {
                            int args_sz = 0;
                            trav++;
                            while (command[trav] != '\0')
                            {
                                args[args_sz++] = command[trav++];
                            }
                            args[args_sz - 1] = '\0';

                            for (int i = 0; i < 2; i++)
                            {
                                if (strcmp(args, info[i].name) == 0)
                                {
                                    strcpy(err_code, "200");
                                    matched_user = i;
                                    fcmd = 0;
                                    break;
                                }
                            }
                            if (fcmd)
                            {
                                strcpy(err_code, "500");
                            }
                        }
                        else
                        {
                            strcpy(err_code, "600");
                        }
                    }
                    else if (scmd)
                    {
                        if (!strcmp(cmdName, "pass"))
                        {
                            int args_sz = 0;
                            trav++;
                            while (command[trav] != '\0')
                            {
                                args[args_sz++] = command[trav++];
                            }
                            args[args_sz - 1] = '\0';

                            if (strcmp(info[matched_user].password, args) == 0)
                            {
                                strcpy(err_code, "200");
                                scmd = 0;
                            }
                            if (scmd)
                                strcpy(err_code, "500");
                        }
                        else
                        {
                            strcpy(err_code, "600");
                        }
                        if (scmd)
                            fcmd = 1;
                    }
                    send(newsockfd, err_code, strlen(err_code), 0);
                    if (!fcmd && !scmd)
                    {
                        printf("[+] User succesfully logged onto the server!\n");
                        break;
                    }
                }
                printf("\n------------------- File-operation commands! -------------------\n\n");
                while (1)
                {
                    // Reset variable arrays
                    memset(command, '\0', sizeof(command));
                    memset(cmdName, '\0', sizeof(cmdName));
                    memset(local_file, '\0', sizeof(local_file));
                    memset(remote_file, '\0', sizeof(remote_file));
                    memset(fnames, '\0', sizeof(fnames));

                    // Receive command from the server
                    int cmdSize = recv(newsockfd, command, MAXSIZE, 0);
                    command[cmdSize - 1] = '\0';

                    // Find command name
                    int trav = findCommandName(command, cmdName);

                    // Implementation of cd
                    if (!strcmp(cmdName, "cd"))
                    {
                        char currDir[MAXSIZE];
                        int args_sz = 0;
                        trav++;

                        while (command[trav] != '\0')
                        {
                            args[args_sz++] = command[trav++];
                        }
                        args[args_sz - 1] = '\0';

                        int ch_dir = chdir(strdup(args));
                        // Error in changing directory
                        if (ch_dir == 0)
                        {
                            strcpy(err_code, "200");
                        }
                        else
                        {
                            strcpy(err_code, "500");
                        }
                        getcwd(currDir, MAXSIZE);
                        printf("\nCurrent working directory: %s\n\n", currDir);
                        send(newsockfd, err_code, strlen(err_code), 0);
                    }
                    // Implementation of dir
                    else if (!strcmp(cmdName, "dir"))
                    {
                        int empty = 1;
                        char null = '\0';
                        DIR *curr_dir;
                        struct dirent *content;
                        char fname[MAXSIZE], currDir[MAXSIZE];
                        curr_dir = opendir(".");
                        getcwd(currDir, MAXSIZE);
                        if (curr_dir)
                        {
                            while ((content = readdir(curr_dir)) != NULL)
                            {
                                if (!strcmp(content->d_name, ".") || !strcmp(content->d_name, ".."))
                                    continue;
                                empty = 0;
                                strcpy(fname, content->d_name);
                                send(newsockfd, fname, strlen(fname) + 1, 0);
                            }
                            if (empty)
                                send(newsockfd, &null, sizeof(char), 0);
                            send(newsockfd, &null, sizeof(char), 0);
                            closedir(curr_dir);
                        }
                        else
                        {
                            printf("ERROR: Cannot open current directory!\n");
                        }
                        continue;
                    }
                    // Implementation of get
                    if (!strcmp(cmdName, "get"))
                    {
                        trav++;
                        int type = 1, i = 0, j = 0;
                        while (command[trav] != '\0')
                        {
                            if ((command[trav] == ' ') && (type == 1))
                            {
                                remote_file[i] = '\0';
                                type = 2;
                                trav++;
                            }
                            else if (type == 1)
                            {
                                remote_file[i++] = command[trav++];
                            }
                            else if (type == 2)
                            {
                                local_file[j++] = command[trav++];
                            }
                        }
                        local_file[j - 1] = '\0'; 
                        int file_desc = open(remote_file, O_RDONLY);
                        if (file_desc < 0)
                        {
                            strcpy(err_code, "500");
                            send(newsockfd, err_code, strlen(err_code), 0);
                            continue;
                        }
                        else
                        {
                            strcpy(err_code, "200");
                            send(newsockfd, err_code, strlen(err_code), 0);

                            char file_buff[CHUNKSIZE], header;
                            // File sending module
                            while ((bytes = read(file_desc, file_buff, CHUNKSIZE)) > 0)
                            {
                                // Convert from Host to Network Long
                                int netBytes = htonl(bytes);
                                file_buff[bytes] = '\0';
                                if (bytes == CHUNKSIZE)
                                    header = 'N';
                                else
                                    header = 'L';

                                // Send character
                                send(newsockfd, &header, sizeof(header), 0);
                                // Send bytes read
                                send(newsockfd, &netBytes, sizeof(int), 0);
                                // Send filebuffer
                                send(newsockfd, file_buff, strlen(file_buff), 0);

                                // If file is completely read break
                                if (bytes < CHUNKSIZE)
                                    break;
                            }
                            close(file_desc);
                        }
                    }
                    // Implementation of put
                    if (!strcmp(cmdName, "put"))
                    {
                        trav++;
                        int type = 1, i = 0, j = 0;
                        while (command[trav] != '\0')
                        {
                            if ((command[trav] == ' ') && (type == 1))
                            {
                                local_file[i] = '\0';
                                type = 2;
                                trav++;
                            }
                            else if (type == 1)
                            {
                                local_file[i++] = command[trav++];
                            }
                            else if (type == 2)
                            {
                                remote_file[j++] = command[trav++];
                            }
                        }
                        int file_desc = open(remote_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                        if (file_desc < 0)
                        {
                            strcpy(err_code, "500");
                            // Send error code
                            send(newsockfd, err_code, strlen(err_code), 0);
                            continue;
                        }
                        strcpy(err_code, "200");
                        // Send error code
                        send(newsockfd, err_code, strlen(err_code), 0);

                        // Receive blocks of data
                        char header;
                        int block_size = 0;
                        unsigned int recv_bytes = 0;
                        while (1)
                        {
                            // Receive character
                            int temp = recv(newsockfd, &header, 1, 0);
                            // Receive number of bytes
                            recv(newsockfd, &block_size, sizeof(int), 0);
                            block_size = ntohl(block_size);
                            char file_buff[block_size];

                            // Receive buffer
                            bytes = recv(newsockfd, file_buff, sizeof(file_buff), 0);
                            file_buff[bytes] = '\0';
                            write(file_desc, file_buff, strlen(file_buff));

                            // Check for last block
                            if ((header == 'L') || (temp <= 0) || (bytes <= 0))
                                break;
                        }
                        close(file_desc);
                    }
                    // Implementation of mget
                    if (!strcmp(cmdName, "mget"))
                    {
                        trav++;
                        int i = 0, j = 0;
                        while (command[trav] != '\0')
                        {
                            if (command[trav] == ',')
                            {
                                fnames[i][j] = '\0';
                                j = 0;
                                i++;
                                trav++;
                            }
                            else if (command[trav] == ' ')
                            {
                                trav++;
                            }
                            else
                            {
                                fnames[i][j] = command[trav];
                                j++;
                                trav++;
                            }
                        }
                        int breakFlg = 0;
                        for (int k = 0; k <= i; k++)
                        {
                            // File transfer begins here
                            int file_desc = open(fnames[k], O_RDONLY);

                            // Error in opening of file
                            if (file_desc < 0)
                            {
                                strcpy(err_code, "500");
                                send(newsockfd, err_code, strlen(err_code), 0);
                                breakFlg = 1;
                                break;
                            }
                            else
                            {
                                strcpy(err_code, "200");
                                send(newsockfd, err_code, strlen(err_code), 0);

                                char file_buff[CHUNKSIZE], header;
                                // File sending module
                                while ((bytes = read(file_desc, file_buff, CHUNKSIZE)) > 0)
                                {
                                    // Convert from Host to Network Long
                                    int netBytes = htonl(bytes);
                                    file_buff[bytes] = '\0';
                                    if (bytes == CHUNKSIZE)
                                        header = 'N';
                                    else
                                        header = 'L';

                                    // Send character
                                    send(newsockfd, &header, sizeof(header), 0);
                                    // Send bytes read
                                    send(newsockfd, &netBytes, sizeof(int), 0);
                                    // Send filebuffer
                                    send(newsockfd, file_buff, strlen(file_buff), 0);

                                    // If file is completely read break
                                    if (bytes < CHUNKSIZE)
                                        break;
                                }
                                close(file_desc);
                            }
                        }
                        if (breakFlg)
                        {
                            printf("\n[-] File transfer unsuccessful!\n");
                        }
                        else
                        {
                            printf("\n[+] File transfer succesful!\n");
                        }
                    }
                    // Implementation of mput
                    if (!strcmp(cmdName, "mput"))
                    {
                        trav++;
                        int i = 0, j = 0;
                        while (command[trav] != '\0')
                        {
                            if (command[trav] == ',')
                            {
                                fnames[i][j] = '\0';
                                j = 0;
                                i++;
                                trav++;
                            }
                            else if (command[trav] == ' ')
                            {
                                trav++;
                            }
                            else
                            {
                                fnames[i][j] = command[trav];
                                j++;
                                trav++;
                            }
                        }
                        int breakFlg = 0;
                        for (int k = 0; k <= i; k++)
                        {
                            int file_desc = open(fnames[k], O_WRONLY | O_CREAT | O_TRUNC, 0666), breakFlg = 0;
                            if (file_desc < 0)
                            {
                                strcpy(err_code, "500");
                                // Send error code
                                send(newsockfd, err_code, strlen(err_code), 0);
                                breakFlg = 1;
                                continue;
                            }
                            else
                            {
                                strcpy(err_code, "200");
                                // Send error code
                                send(newsockfd, err_code, strlen(err_code), 0);

                                // Receive blocks of data
                                char header;
                                int block_size = 0;
                                unsigned int recv_bytes = 0;
                                while (1)
                                {
                                    // Receive character
                                    int temp = recv(newsockfd, &header, 1, 0);
                                    // Receive number of bytes
                                    recv(newsockfd, &block_size, sizeof(int), 0);
                                    block_size = ntohl(block_size);
                                    char file_buff[block_size];

                                    // Receive buffer
                                    bytes = recv(newsockfd, file_buff, sizeof(file_buff), 0);
                                    file_buff[bytes] = '\0';
                                    write(file_desc, file_buff, strlen(file_buff));

                                    // Check for last block
                                    if ((header == 'L') || (temp <= 0) || (bytes <= 0))
                                        break;
                                }
                                close(file_desc);
                            }
                        }
                        if (breakFlg)
                        {
                            printf("\n[-] File transfer unsuccessful!\n");
                        }
                        else
                        {
                            printf("\n[+] File transfer succesful!\n");
                        }
                    }
                }
                // Runnable code .... (END)
                close(newsockfd);
                exit(EXIT_SUCCESS);
            }
            close(newsockfd);
        }
    }
    close(server_fd);
    return 0;
}
