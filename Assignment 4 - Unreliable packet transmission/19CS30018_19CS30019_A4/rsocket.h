// Group 4
// Gaurav Madkaikar - 19CS30018
// Girish Kumar - 19CS30019

#ifndef RSOCKET_H
#define RSOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>  
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h> 
#include <signal.h>
#include <time.h>  
#include <pthread.h> 

// Constants
// Maximum size of the input message
#define MSG_SIZE 100
// Maximum size of UMT and RMT tables
#define TABLE_SIZE 50
// Sleeping time T (2 sec)
#define T 2
// SOCK_MRP
#define SOCK_MRP 686
// Timeout = 4 sec
#define MSG_TIMEOUT 4
// Drop probability (Default value = 0.2)
#define DROP_PROBABILITY 0.2

// Unacknowledged Message Table
typedef struct unacknowledged_message_table
{
    int id;
    char buffer[MSG_SIZE];
    size_t buffer_len;
    time_t time;
    int flag;
    struct sockaddr_in dest_addr;
    socklen_t dest_addr_len;
} unackMsgTable;
unackMsgTable *UMT;

// Received Message Table
typedef struct received_message_table
{
    int id;
    char buffer[MSG_SIZE];
    struct sockaddr_in sender_addr;
    socklen_t addr_len;
} recvMsgTable;
recvMsgTable *RMT;

int r_socket(int __domain, int __type, int __protocol);

int r_bind(int __fd, const struct sockaddr *__addr, socklen_t __len);

ssize_t r_sendto(int __fd, const void *buf, size_t __n, int __flags, const struct sockaddr *__addr, socklen_t __addr_len);

ssize_t r_recvfrom(int __fd, char *buf, size_t __n, int __flags, struct sockaddr *__addr, socklen_t *__addr_len);

int r_close(int __fd);

int dropMessage(float p);

#endif