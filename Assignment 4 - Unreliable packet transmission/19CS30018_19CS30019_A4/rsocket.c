// Group 4
// Gaurav Madkaikar - 19CS30018
// Girish Kumar - 19CS30019
#include "rsocket.h"

// ----------- Global variables -----------
// Mutex lock
pthread_mutex_t lock;

// Threads R and S
pthread_t Sid, Rid;

// Socket descriptor
int sock_fd = -1;

// Count the number of transitions
int numTransmissions = 0;
int msgSequence = 0;
// Number of buffer items received in RMT
int dataRecv = 0;
int recvFlag = 0;

// ----------- Auxiliary methods -----------
// Delete message from UMT
int deleteAckMsg(int id)
{
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        if (UMT[i].id == id)
        {
            UMT[i].id = -1;
            return 0;
        }
    }
    return -1;
}

// Drop message
int dropMessage(float p)
{
    float randNum = (float)rand() / RAND_MAX;
    return (randNum < p);
}

// Add sequence number to the data buffer
void addSequenceNum(char *buf, int __id, int __len)
{
    int tot_len = __len + sizeof(int);
    for (int i = __len; i < tot_len; i++)
        buf[i] = '\0';
    strcat(buf + __len + 1, (char *)&__id);
}

// Send acknowledgement
ssize_t sendACKMsg(int __id, struct sockaddr_in __addr, socklen_t __addr_len)
{
    char ack_msg[MSG_SIZE];
    strcpy(ack_msg, "ACK");
    addSequenceNum(ack_msg, __id, strlen(ack_msg));
    size_t tot_len = sizeof(int) + strlen(ack_msg);
    size_t sentBytes = sendto(sock_fd, ack_msg, tot_len, 0, (struct sockaddr *)&__addr, __addr_len);
    return sentBytes;
}

// Retrieve message ID from data buffer
void getMessageID(int *__id, char *buf, int __len)
{
    int *msgId;
    __len = strlen(buf);
    msgId = (int *)(buf + __len + 1);
    *__id = *msgId;
}

// Add data message to the RMT
int addDataMsg(int __id, char *buf, struct sockaddr_in __addr, socklen_t __addr_len)
{
    int msgPresent = 0, insertPos = -1;

    // Insert message into the RMT
    // Find empty space in the RMT table
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        if (RMT[i].id == -1)
        {
            insertPos = i;
            break;
        }
    }
    if (insertPos == -1)
        return -1;
    RMT[insertPos].id = __id;
    RMT[insertPos].sender_addr = __addr;
    RMT[insertPos].addr_len = __addr_len;
    strcpy(RMT[insertPos].buffer, buf);
    dataRecv++;

    // Send an acknowledgment to the sender
    sendACKMsg(__id, __addr, __addr_len);
    return 0;
}

// Re-transmit timed out messages 
void reTransmitMsg(int expiredId)
{
    // Retransmit the expired buffer
    printf("[-] Timeout: retransmit msgId - %d\n", UMT[expiredId].id);
    ssize_t sentBytes = sendto(sock_fd, UMT[expiredId].buffer, UMT[expiredId].buffer_len, UMT[expiredId].flag, (struct sockaddr *)&UMT[expiredId].dest_addr, UMT[expiredId].dest_addr_len);
    numTransmissions++;
}

// Initialise UMT table
void init_UMT_table(int index)
{
    UMT[index].id = -1;
    UMT[index].dest_addr_len = 0;
    memset(UMT[index].buffer, '\0', sizeof(UMT[index].buffer));
    UMT[index].buffer_len = 0;
    UMT[index].flag = 0;
    UMT[index].time = time(0);
}

// Initialise RMT table
void init_RMT_table(int index)
{
    RMT[index].id = -1;
    RMT[index].addr_len = 0;
    memset(RMT[index].buffer, '\0', sizeof(RMT[index].buffer));
}

// Delete entry from RMT
void deleteRMTEntry(int msgId)
{
    RMT[msgId].id = -1;
    memset(RMT[msgId].buffer, '\0', strlen(RMT[msgId].buffer));
    dataRecv--;
}

// ----------- Main Methods -----------

// Thread R handles all messages received from the UDP socket
void *RthreadRunner(void *args)
{
    int id, statusOp = -1;
    char buffer[MSG_SIZE];
    struct sockaddr_in source_addr;
    memset(buffer, '\0', sizeof(buffer));
    socklen_t addr_len = sizeof(source_addr);
    while (1)
    {
        // Wait for a message to arrive (block until then)
        int recvBytes = recvfrom(sock_fd, buffer, MSG_SIZE, recvFlag, (struct sockaddr *)&source_addr, &addr_len);
        buffer[recvBytes] = '\0';
        if (dropMessage(DROP_PROBABILITY))
        {
            // Don't do anything
            continue;
        }
        getMessageID(&id, buffer, recvBytes);
        // Data received:
        // 1) Acknowledgement message: Remove the message for which ACK has arrived
        pthread_mutex_lock(&lock);
        if (strcmp(buffer, "ACK") == 0)
        {
            statusOp = deleteAckMsg(id);
            if (statusOp == 0)
            {
                printf("[+] Acknowledgement: msgId - %d\n", id);
            }
        }
        // 2) Data message: Store in RMT and send an acknowledgement to sender
        else
        {
            statusOp = addDataMsg(id, buffer, source_addr, addr_len); //, printf("Added to RMT: %d!\n", dataRecv);
        }
        pthread_mutex_unlock(&lock);
    }
    pthread_exit(0);
}

// Thread S handles the timeouts and retransmissions
void *SthreadRunner(void *args)
{
    time_t timeout = MSG_TIMEOUT;
    while (1)
    {
        // sleep for time T sec
        sleep(T);
        pthread_mutex_lock(&lock);
        // Check UMT for timer expiry
        for (int i = 0; i < TABLE_SIZE; i++)
        {
            // Valid id and timeout results in retransmission
            if ((UMT[i].id != -1) && ((time(0) - UMT[i].time) > timeout))
            {
                reTransmitMsg(i);
                UMT[i].time = time(0);
                break;
                
            }
        }
        pthread_mutex_unlock(&lock);
    }
    pthread_exit(0);
}

// Function to create the socket connection
int r_socket(int __domain, int __type, int __protocol)
{
    srand(time(0));
    
    if (__type != SOCK_MRP)
        return -1;

    if ((sock_fd = socket(__domain, SOCK_DGRAM, __protocol)) < 0)
        return sock_fd;

    // Initialise tables
    UMT = (unackMsgTable *)malloc(TABLE_SIZE * sizeof(unackMsgTable));
    RMT = (recvMsgTable *)malloc(TABLE_SIZE * sizeof(recvMsgTable));
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        init_RMT_table(i);
        init_UMT_table(i);
    }

    // Initialise mutex locks
    pthread_mutex_init(&lock, NULL);

    // Initialise threads R and S
    int R = pthread_create(&Rid, NULL, RthreadRunner, NULL);
    if (R < 0)
        return -1;
    int S = pthread_create(&Sid, NULL, SthreadRunner, NULL);
    if (S < 0)
        return -1;

    return sock_fd;
}

// Function to bind socket to the server
int r_bind(int __fd, const struct sockaddr *__addr, socklen_t __addr_len)
{
    int status = bind(__fd, __addr, __addr_len);
    return status;
}

// Function to take message from sender and transmit to receiver (blocking in nature)
ssize_t r_sendto(int __fd, const void *buf, size_t __len, int flag, const struct sockaddr *__addr, socklen_t __addr_len)
{
    // Invalid connection or invalid message length
    if ((sock_fd < 0) || (__len < 0))
    {
        return -1;
    }
    int msgId = ++msgSequence;
    char *buffer = (char *)buf;

    unackMsgTable *ptrMsg = NULL;
    // Store the message to the unacknowledged message table (UMT)
    // Find an empty place in the table
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        if (UMT[i].id == -1)
        {
            ptrMsg = &UMT[i];
            break;
        }
    }
    // No empty place found
    if (ptrMsg == NULL)
        return -1;

    // Add entry to the UMT
    ptrMsg->id = msgId;
    strcpy(ptrMsg->buffer, buffer);
    // Add sequence number to the message
    addSequenceNum(ptrMsg->buffer, ptrMsg->id, __len);
    ptrMsg->buffer_len = __len + sizeof(int);
    ptrMsg->flag = flag;
    ptrMsg->dest_addr = *(struct sockaddr_in *)__addr;
    ptrMsg->dest_addr_len = __addr_len;
    ptrMsg->time = time(0);

    // Send buffer through sockfd
    ssize_t sentBytes = sendto(__fd, ptrMsg->buffer, ptrMsg->buffer_len, flag, __addr, __addr_len);
    numTransmissions++;

    sleep(1);
    return sentBytes;
}

// Receive data from sender
ssize_t r_recvfrom(int __fd, char *buf, size_t __len, int flags, struct sockaddr *__addr, socklen_t *__addr_len)
{
    int breakFlg = 0;
    if (__fd != sock_fd)
        return -1;
    // Periodically look up the RMT for any received message
    while (1)
    {
        if (__len < 0)
            return -1;
        // If any message is available in the RMT, return it and delete
        if ((dataRecv > 0) || (numTransmissions > 0))
        {
            int bufIndex = -1;
            // Find the first message
            for (int i = 0; i < TABLE_SIZE; i++)
            {
                if (RMT[i].id != -1)
                {
                    bufIndex = i;
                    break;
                }
            }
            printf("\n");
            strcpy(buf, RMT[bufIndex].buffer);
            if ((__len >= 0) && (__len <= strlen(buf)))
            {
                buf[__len] = '\0';
            }
            __len = strlen(RMT[bufIndex].buffer);
            recvFlag = flags;
            
            // Delete the first message in RMT
            deleteRMTEntry(bufIndex);

            return __len;
        }
        // Block the call for sometime
        else
        {
            // Sleep for 1 second
            sleep(1);
        }
    }
    return 0;
}

// Close connection and free acquired memory
int r_close(int __fd)
{
    if (__fd != sock_fd)
        return -1;

    // All unacknowledged messages must be deleted
    while (1)
    {
        int closeFlg = 0;
        for (int i = 0; i < TABLE_SIZE; i++)
            if (UMT[i].id != -1)
                closeFlg = 1;
        if (!closeFlg)
            break;
    }
    // Print the number of transmissions
    printf("No of transmissions = %d\n", numTransmissions);

    // Kill both threads
    pthread_kill(Sid, SIGKILL);
    pthread_kill(Rid, SIGKILL);

    // Free the tables
    free(UMT);
    free(RMT);

    // Close the socket
    return close(__fd);
}