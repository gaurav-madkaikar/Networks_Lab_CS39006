// Group 4
// Gaurav Madkaikar (19CS30018)
// Girish Kumar (19CS30019)
// FTP Client

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <ctype.h>

#define MAXSIZE 300
#define MAXBUFFSIZE 50
#define ERR_SIZE 3

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

int validate_number(char *str)
{
    while (*str)
    {
        // If each string is not an unsigned integer
        if (!isdigit(*str))
        { 
            return 0;
        }
        // point to the next character
        str++; 
    }
    return 1;
}

int validate_IP(char *ip)
{
    int i, num, dots = 0;
    char *ptr;
    if (ip == NULL)
        return 0;
    // Cut the string using a delimiter ('.')
    strcpy(ptr, strtok(ip, ".")); 
    if (ptr == NULL)
        return 0;
    while (ptr)
    {
        // Check whether the sub string is holding only number or not
        if (!validate_number(ptr))
            return 0; 
        // Cnvert substring to number
        num = atoi(ptr); 
        if (num >= 0 && num <= 255)
        {
            // Cut the next part of the string
            ptr = strtok(NULL, "."); 
            if (ptr != NULL)
                dots++;         // Increase the dot count
        }
        else
            return 0;
    }
    // If the number of dots are not 3, return false
    if (dots != 3)
        return 0; 

    return 1;
}

int main(int argc, char *argv[])
{
    printf("--------------- FTP Client Side ---------------\n\n");
    char client_cmd[MAXSIZE];
    char arg[MAXSIZE], *pch, *IP_address;
    char *prt;
    unsigned int port;
    int first = 1, client_fd1;
    struct sockaddr_in servaddr;

    // Check the validity of the first command (client side)
    // Command Prompt
    printf("myFTP> ");
    scanf("%s", client_cmd);
    fgets(arg, sizeof(arg), stdin);

    if (strcmp(client_cmd, "open") == 0)
    {
        pch = strtok(arg, " ");
        while (pch != NULL)
        {
            IP_address = pch;
            pch = strtok(NULL, " ");
            prt = pch;
            pch = strtok(NULL, " ");
        }
        port = atoi(prt);
        if (validate_IP(strdup(IP_address)) == 0)
        {
            printf("ERROR: Invalid IP Address!\n");
            exit(EXIT_FAILURE);
        }

        // Server Information
        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(port);
        if ((port < 20000) || (port > 65535) || ((servaddr.sin_addr.s_addr = inet_addr(IP_address)) == -1))
        {
            printf("ERROR: Invalid port! (Enter a port value between 20000 and 65535)\n");
            exit(EXIT_FAILURE);
        }

        printf("Server IP address: %s\nMachine Port : %d\n", IP_address, port);
    }
    else
    {
        printf("ERROR: Invalid first command!\n");
        exit(EXIT_FAILURE);
    }

    // Create a socket
    if ((client_fd1 = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    // Reuse the port if not available
    if (setsockopt(client_fd1, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) < 0)
    {
        perror("Port cannot be reused");
        exit(EXIT_FAILURE);
    }

    // Estabilish a connection to the server
    if (connect(client_fd1, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("Cannot connect to the server");
        exit(EXIT_FAILURE);
    }
    printf("\n[+] Connection opened successfully!\n");

    char server_cmd[MAXSIZE], err_code[ERR_SIZE], cmdName[MAXSIZE], args[MAXSIZE];
    int trav = 0, cmd_type = 1, bytes = 0;

    // User Authentication
    while (1)
    {
        // Command Prompt
        printf("\nmyFTP> ");
        fgets(server_cmd, sizeof(server_cmd), stdin);

        // send command to the server
        send(client_fd1, server_cmd, strlen(server_cmd), 0);

        // receive error code
        bytes = recv(client_fd1, err_code, ERR_SIZE, 0);
        err_code[bytes] = '\0';

        if (!strcmp(err_code, "200"))
        {
            // Valid username
            if (cmd_type == 1)
            {
                cmd_type = 2;
                printf("[+] Username matched!\n");
            }
            // Valid username & password
            else if (cmd_type == 2)
            {
                printf("[+] Password matched!\n");
                break;
            }
        }
        else if (!strcmp(err_code, "500"))
        {
            // User not found
            if (cmd_type == 1)
                printf("ERROR: Requested user not found!\n");
            else if (cmd_type == 2)
                printf("ERROR: Entered password does not match!\n"), cmd_type = 1;
        }
        else
        {
            printf("ERROR: Invalid ");
            if (cmd_type == 1)
            {
                printf("first ");
            }
            else
            {
                printf("second "), cmd_type = 1;
            }
            printf("command sent to the server!\n");
        }
    }
    // File-based Operations
    while (1)
    {
        char fnames[MAXSIZE][MAXSIZE], remote_file[MAXSIZE], local_file[MAXSIZE], currDir[MAXSIZE];

        // Command Prompt
        printf("\nmyFTP> ");
        fgets(server_cmd, sizeof(server_cmd), stdin);

        // Find name of the command
        trav = findCommandName(server_cmd, cmdName);

        // ---------------- Commands that do not send data to the server ----------------
        // Local (client-side) directory change (lcd)
        if (!strcmp(cmdName, "lcd"))
        {
            int args_sz = 0;
            trav++;
            while (server_cmd[trav] != '\0')
            {
                if (server_cmd[trav] == '\n')
                {
                    trav++;
                    continue;
                }
                args[args_sz++] = server_cmd[trav++];
            }
            args[args_sz] = '\0';
            int ch_dir = chdir(strdup(args));
            if (ch_dir == 0)
            {
                printf("[+] Local (client-side) directory successfully changed!\n");
            }
            else
            {
                printf("[-] Local (client-side) directory cannot be changed!\n");
            }
            getcwd(currDir, MAXSIZE);
            printf("Current local working directory: %s\n\n", currDir);
            continue;
        }

        // quit (Close the connection)
        else if (!strcmp(cmdName, "quit"))
        {
            close(client_fd1);
            printf("\nExiting TCP client ....\n");
            break;
        }

        // cd 
        else if (!strcmp(cmdName, "cd"))
        {
            // Send command to the server
            send(client_fd1, server_cmd, strlen(server_cmd), 0);

            // receive error code
            bytes = recv(client_fd1, err_code, ERR_SIZE, 0);
            err_code[bytes] = '\0';

            if (!strcmp(err_code, "200"))
            {
                printf("[+] Directory change successful!\n");
            }
            else
            {
                printf("[-] Directory change unsuccessful!\n");
            }
        }
        // dir 
        else if (!strcmp(cmdName, "dir"))
        {
            // Send command to the server
            send(client_fd1, server_cmd, strlen(server_cmd), 0);

            char buffer[MAXBUFFSIZE], prev_char = ' ';
            int ptr = 0, flg = 0, per_row = 8, cnt = 0, chseen = 0;
            int bytes;
            while ((bytes = recv(client_fd1, buffer, MAXSIZE, 0)) > 0)
            {
                ptr++;
                for (int i = 0; i < bytes; i++)
                {
                    if ((buffer[i] == '\0') && (prev_char == '\0'))
                    {
                        printf("\n");
                        flg = 1;
                        break;
                    }
                    else if (buffer[i] == '\0')
                    {
                        cnt++;
                        if (cnt == per_row)
                            printf("\n"), cnt = 0;
                        else
                            printf("    ");
                    }
                    else
                    {
                        chseen = 1;
                        printf("\e[1;97m%c\e[0m", buffer[i]);
                    }
                    prev_char = buffer[i];
                }
                if (flg)
                    break;
            }
            if (!chseen)
            {
                printf("[+] No files/sub-directories present in the current server directory!\n");
                continue;
            }
        }
        // get
        else if (!strcmp(cmdName, "get"))
        {
            trav++;
            int type = 1, i = 0, j = 0;
            while (server_cmd[trav] != '\0')
            {
                if ((server_cmd[trav] == ' ') && (type == 1))
                {
                    remote_file[i] = '\0';
                    type = 2;
                    trav++;
                }
                else if (type == 1)
                {
                    remote_file[i++] = server_cmd[trav++];
                }
                else if (type == 2)
                {
                    local_file[j++] = server_cmd[trav++];
                }
            }
            local_file[j - 1] = '\0';
            // printf("Local File: %s %d\nRemote File: %s %d\n", local_file, (int)strlen(local_file), remote_file, (int)strlen(remote_file));
            // Create or overwrite file
            int file_desc = open(local_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (file_desc < 0)
            {
                printf("ERROR: File cannot be opened for writing!\n");
                continue;
            }
            printf("[+] Local file opened for writing successfully!\n");

            // Send command to the server
            send(client_fd1, server_cmd, strlen(server_cmd), 0);

            // receive error code
            bytes = recv(client_fd1, err_code, ERR_SIZE, 0);
            err_code[bytes] = '\0';

            if (!strcmp(err_code, "500"))
            {
                printf("ERROR: Remote file does not exist in the server directory!\n");
                close(file_desc);
                continue;
            }
            else if (!strcmp(err_code, "200"))
            {
                // Receive blocks of data
                char header;
                int block_size = 0;
                unsigned int recv_bytes = 0;
                while (1)
                {
                    // Receive character
                    int temp = recv(client_fd1, &header, 1, 0);
                    // Receive number of bytes
                    recv(client_fd1, &block_size, sizeof(int), 0);
                    block_size = ntohl(block_size);
                    char file_buff[block_size];

                    // Receive buffer
                    bytes = recv(client_fd1, file_buff, sizeof(file_buff), 0);
                    file_buff[bytes] = '\0';
                    write(file_desc, file_buff, strlen(file_buff));

                    // Check for last block
                    if ((header == 'L') || (temp <= 0) || (bytes <= 0))
                        break;
                }
                close(file_desc);
            }
        }
        // put
        else if (!strcmp(cmdName, "put"))
        {
            trav++;
            int type = 1, i = 0, j = 0;
            while (server_cmd[trav] != '\0')
            {
                if ((server_cmd[trav] == ' ') && (type == 1))
                {
                    local_file[i] = '\0';
                    type = 2;
                    trav++;
                }
                else if (type == 1)
                {
                    local_file[i++] = server_cmd[trav++];
                }
                else if (type == 2)
                {
                    remote_file[j++] = server_cmd[trav++];
                }
            }
            remote_file[j - 1] = '\0';

            // Open file for reading
            int file_desc = open(local_file, O_RDONLY);
            if (file_desc < 0)
            {
                printf("ERROR: File cannot be opened for reading!\n");
                continue;
            }
            printf("[+] Local file opened for reading successfully!\n");

            // Send command to the server
            send(client_fd1, server_cmd, strlen(server_cmd), 0);

            // Receive error code
            bytes = recv(client_fd1, err_code, ERR_SIZE, 0);
            err_code[bytes] = '\0';

            if (!strcmp(err_code, "200"))
            {
                char file_buff[15], header;
                // File sending module
                while ((bytes = read(file_desc, file_buff, 15)) > 0)
                {
                    // Convert from Host to Network Long
                    int netBytes = htonl(bytes);
                    file_buff[bytes] = '\0';
                    if (bytes == 15)
                        header = 'N';
                    else
                        header = 'L';

                    // Send character
                    send(client_fd1, &header, sizeof(header), 0);
                    // Send bytes read
                    send(client_fd1, &netBytes, sizeof(int), 0);
                    // Send filebuffer
                    send(client_fd1, file_buff, strlen(file_buff), 0);

                    // If file is completely read break
                    if (bytes < 15)
                        break;
                }
                printf("[+] File transfer successful!\n");
                close(file_desc);
            }
            else if (!strcmp(err_code, "500"))
            {
                printf("ERROR: Server is unable to open the remote file!\n");
                close(file_desc);
            }
        }
        // mget
        else if (!strcmp(cmdName, "mget"))
        {
            memset(fnames, '\0', sizeof(fnames));
            // Send command to the server
            send(client_fd1, server_cmd, strlen(server_cmd), 0);

            int i = 0, j = 0;
            while (server_cmd[trav] != '\0')
            {
                if (server_cmd[trav] == ',')
                {
                    fnames[i][j] = '\0';
                    j = 0;
                    i++;
                    trav++;
                }
                else if (server_cmd[trav] == ' ')
                {
                    trav++;
                }
                else
                {
                    fnames[i][j] = server_cmd[trav];
                    j++;
                    trav++;
                }
            }
            fnames[i][j - 1] = '\0';
            int breakFlg = 0;
            for (int k = 0; k <= i; k++)
            {
                int file_desc;

                // Receive error code
                bytes = recv(client_fd1, err_code, ERR_SIZE, 0);
                err_code[bytes] = '\0';

                if (!strcmp(err_code, "500"))
                {
                    printf("ERROR: Invalid file transfer!\n");
                    close(file_desc);
                    breakFlg = 1;
                    break;
                }
                else if (!strcmp(err_code, "200"))
                {
                    file_desc = open(fnames[k], O_WRONLY | O_CREAT | O_TRUNC, 0666);
                    // Receive blocks of data
                    char header;
                    int block_size = 0;
                    unsigned int recv_bytes = 0;
                    while (1)
                    {
                        // Receive character
                        int temp = recv(client_fd1, &header, 1, 0);
                        // Receive number of bytes
                        recv(client_fd1, &block_size, sizeof(int), 0);
                        block_size = ntohl(block_size);
                        char file_buff[block_size];

                        // Receive buffer
                        bytes = recv(client_fd1, file_buff, sizeof(file_buff), 0);
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
                printf("\n[+] File transfer successful!\n");
            }
        }
        // mput
        else if (!strcmp(cmdName, "mput"))
        {
            memset(fnames, '\0', sizeof(fnames));

            int i = 0, j = 0;
            while (server_cmd[trav] != '\0')
            {
                if (server_cmd[trav] == ',')
                {
                    fnames[i][j] = '\0';
                    j = 0;
                    i++;
                    trav++;
                }
                else if (server_cmd[trav] == ' ')
                {
                    trav++;
                }
                else
                {
                    fnames[i][j] = server_cmd[trav];
                    j++;
                    trav++;
                }
            }
            fnames[i][j - 1] = '\0';
            int breakFlg = 0;
            for (int k = 0; k <= i; k++)
            {
                // Open file for reading
                int file_desc = open(fnames[k], O_RDONLY);
                if (file_desc < 0)
                {
                    printf("ERROR: Client cannot open %s for reading!\n", fnames[k]);
                    breakFlg = 1;
                    break;
                }
            }
            if (breakFlg)
                continue;
            breakFlg = 0;

            // Send command to the server
            send(client_fd1, server_cmd, strlen(server_cmd), 0);

            printf("[+] Local files opened for reading successfully!\n");
            for (int k = 0; k <= i; k++) // Send command to the server
            {
                // Open file for reading
                int file_desc = open(fnames[k], O_RDONLY);

                // Receive error code
                bytes = recv(client_fd1, err_code, ERR_SIZE, 0);
                err_code[bytes] = '\0';

                if (!strcmp(err_code, "200"))
                {
                    char file_buff[15], header;
                    // File sending module
                    while ((bytes = read(file_desc, file_buff, 15)) > 0)
                    {
                        // Convert from Host to Network Long
                        int netBytes = htonl(bytes);
                        file_buff[bytes] = '\0';
                        if (bytes == 15)
                            header = 'N';
                        else
                            header = 'L';

                        // Send character
                        send(client_fd1, &header, sizeof(header), 0);
                        // Send bytes read
                        send(client_fd1, &netBytes, sizeof(int), 0);
                        // Send filebuffer
                        send(client_fd1, file_buff, strlen(file_buff), 0);

                        // If file is completely read break
                        if (bytes < 15)
                            break;
                    }
                    close(file_desc);
                }
                else if (!strcmp(err_code, "500"))
                {
                    printf("ERROR: Server is unable to open the remote file!\n");
                    close(file_desc);
                    breakFlg = 1;
                    break;
                }
            }
            if (breakFlg)
            {
                printf("\n[-] File transfer unsuccessful!\n");
            }
            else
            {
                printf("\n[+] File transfer successful!\n");
            }
        }
        else
        {
            printf("ERROR: Invalid command!\n");
        }
    }
    return 0;
}
