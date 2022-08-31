// Networks Lab Assignment 5 - Implementation of mytraceroute
// Gaurav Madkaikar (19CS30018)
// Girish Kumar (19CS30019)

// Takes a maximum of 16 hops (If required this count can be modified by changing the value of MAX_HOPS)
// Instructions to run the code:
// 1. Compile the code as: gcc mytraceroute_19CS30018_19CS30019.c -o mytraceroute
// 2. Run the executable file with sudo permissions: sudo ./mytraceroute www.iitkgp.ac.in

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/wait.h>

#define TIMEOUT 1
#define MAX_IP_LEN 100
#define DATA_SIZE 52
#define MAX_HOPS 16
#define SRC_PORT 10001
#define DEST_PORT 32164
#define PACKET_SIZE 8192

// Get IP Address of the specified hostname
int getIPAddress(char *host, char *ipAddr)
{
    struct hostent *hostEntry;
    struct in_addr **addr_list;
    hostEntry = gethostbyname(host);
    if (hostEntry == NULL)
    {
        perror("gethostbyname");
        return 0;
    }
    addr_list = (struct in_addr **)hostEntry->h_addr_list;
    if (addr_list[0] == NULL)
        return 0;

    strcpy(ipAddr, inet_ntoa(*addr_list[0]));
    return 1;
}

// Function to generate random string as payload
void generatePayload(char *data)
{
    for (int i = 0; i < PACKET_SIZE; i++)
    {
        srand(time(0));
        data[i] = (rand() % 26) + ((rand() % 2) ? 'A' : 'a');
    }
    data[PACKET_SIZE] = '\0';
}

// Generate a random checksum
unsigned short genCheckSum(unsigned short *buffer, int num)
{
    long long finCheckSum = 0;
    for (; num > 0; num--)
        finCheckSum += (*buffer++);
    finCheckSum = (finCheckSum >> 16) + (finCheckSum & 0xffff);
    finCheckSum += (finCheckSum >> 16);
    return (unsigned short)(~finCheckSum);
}

// Generate a IPv4 packet
void generateIPPacket(struct iphdr *ip, int ttl, unsigned dest_addr, char *buffer)
{
    ip->ihl = 5;
    ip->version = 4;
    ip->tot_len = sizeof(struct iphdr) + sizeof(struct udphdr) + DATA_SIZE;
    ip->id = htons(55000);
    ip->ttl = ttl;
    ip->tos = 0;

    // Specify UDP protocol
    ip->protocol = 17;
    ip->daddr = dest_addr;
    ip->saddr = 0;

    // Generate the checksum integer
    ip->check = genCheckSum((unsigned short *)buffer, sizeof(struct iphdr) + sizeof(struct udphdr));
    return;
}

// Generate the UDP header
void generateUDPHeader(struct udphdr *udp, unsigned short src_port, unsigned short dest_port)
{
    udp->source = htons(src_port);
    udp->dest = htons(dest_port);
    udp->len = htons(sizeof(struct udphdr) + DATA_SIZE);
    return;
}

// Main function
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("ERROR: Invalid number of arguments\n");
        exit(EXIT_FAILURE);
    }

    int sockfd_udp, sockfd_icmp;
    struct sockaddr_in source_addr, client_addr;
    socklen_t source_addr_len;

    char dest_IP[MAX_IP_LEN];
    unsigned short src_port, dest_port;
    unsigned dest_addr;

    // Define the required sockets
    sockfd_udp = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    sockfd_icmp = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if ((sockfd_udp < 0) || (sockfd_icmp < 0))
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    // Get the IP Address of the destination
    if (getIPAddress(argv[1], dest_IP) == 0)
    {
        close(sockfd_icmp);
        close(sockfd_udp);
        exit(EXIT_FAILURE);
    }
    // Convert host address from number-and-dot notation to network byte order
    dest_addr = inet_addr(dest_IP);
    src_port = SRC_PORT;
    dest_port = DEST_PORT;

    // Client address info
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(DEST_PORT);
    client_addr.sin_addr.s_addr = dest_addr;

    // Source address info
    source_addr.sin_family = AF_INET;
    source_addr.sin_port = htons(SRC_PORT);
    source_addr.sin_addr.s_addr = INADDR_ANY;
    source_addr_len = sizeof(source_addr);

    // Variables for handling transmission of packets
    int hops = 1, valid = 1, timeout = TIMEOUT;
    fd_set readSockSet;
    int noOfTransmissions = 0;
    char payload[52];
    clock_t start, end;

    // Bind the udp socket to the source address
    if (bind(sockfd_udp, (struct sockaddr *)&source_addr, source_addr_len) < 0)
    {
        perror("bind");
        close(sockfd_udp);
        close(sockfd_icmp);
        exit(EXIT_FAILURE);
    }

    printf("mytraceroute to %s (%s), %d hops max, %d byte packets\n\n", argv[1], dest_IP, MAX_HOPS, DATA_SIZE);
    printf("Hop Count\tIP Address\tResponse Time\n");
    // Avoid blocking of port if already used
    const int yes = 1;
    if ((setsockopt(sockfd_udp, IPPROTO_IP, IP_HDRINCL, &yes, sizeof(yes))) < 0)
    {
        perror("setsockopt");
        close(sockfd_udp);
        close(sockfd_icmp);
        exit(EXIT_FAILURE);
    }

    // Break if maximum number of hops are done
    while (hops <= MAX_HOPS)
    {
        struct timeval tv;
        tv.tv_sec = TIMEOUT;
        tv.tv_usec = 0;

        char dataBuffer[PACKET_SIZE];
        struct iphdr *ip = (struct iphdr *)dataBuffer;
        struct udphdr *udp = (struct udphdr *)(dataBuffer + sizeof(struct iphdr));
        if (valid)
        {
            // Generate random string as payload data
            generatePayload(payload);
            memset(dataBuffer, 0, PACKET_SIZE);

            // Generate the corresponding UDP and IP headers
            generateUDPHeader(udp, src_port, dest_port);
            generateIPPacket(ip, hops, dest_addr, dataBuffer);

            // Increase the number of transmissions
            noOfTransmissions++;

            // Send packet to the destination
            strcpy(dataBuffer + sizeof(struct udphdr) + sizeof(struct iphdr), payload);
            int sentStatus = sendto(sockfd_udp, dataBuffer, ip->tot_len, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
            if (sentStatus < 0)
            {
                perror("sendto");
                close(sockfd_udp);
                close(sockfd_icmp);
                exit(EXIT_FAILURE);
            }

            start = clock();
        }
        // Wait on select call
        FD_ZERO(&readSockSet);
        FD_SET(sockfd_icmp, &readSockSet);

        int waitStatus = select(sockfd_icmp + 1, &readSockSet, 0, 0, &tv);
        if (waitStatus == -1)
        {
            perror("select");
            close(sockfd_icmp);
            close(sockfd_udp);
            exit(EXIT_FAILURE);
        }
        // Select timed out
        else if (waitStatus == 0)
        {
            // Number of repeats exceeded
            if (noOfTransmissions > 3)
            {
                printf("   %d\t\t*\t\t*\n", hops);
                noOfTransmissions = 1;
                hops++;
            }
            valid = 1;
            continue;
        }
        else
        {
            // Handling received ICMP packets
            if (FD_ISSET(sockfd_icmp, &readSockSet))
            {
                // Receive the ICMP Message
                struct iphdr IP_hdr;
                struct icmphdr ICMP_hdr;
                struct in_addr source_add_IP;

                char message[100];
                int recvBytes = recvfrom(sockfd_icmp, message, 2048, 0, (struct sockaddr *)&source_addr, &source_addr_len);
                end = clock();
                if (recvBytes <= 0)
                {
                    timeout = TIMEOUT;
                    valid = 1;
                    continue;
                }

                IP_hdr = *((struct iphdr *)message);
                ICMP_hdr = *((struct icmphdr *)(message + sizeof(IP_hdr)));

                // Read the destination IP
                source_add_IP.s_addr = IP_hdr.saddr;

                // Identify an ICMP packet
                if (IP_hdr.protocol == 1)
                {
                    // ICMP Time Exceeded Message
                    if (ICMP_hdr.type == 11)
                    {
                        // Print the intermediate node
                        printf("   %d\t\t%s\t%.3f ms\n", hops, inet_ntoa(source_add_IP), (float)(end - start) / CLOCKS_PER_SEC * 1000);
                        hops++;
                        noOfTransmissions = 1;
                        timeout = TIMEOUT;
                        valid = 1;
                        continue;
                    }
                    // ICMP Destination Unreachable Message
                    else if (ICMP_hdr.type == 3)
                    {
                        // Check if the source IP address matches with the target server IP Address!
                        if (IP_hdr.saddr == ip->daddr)
                        {
                            printf("   %d\t\t%s\t%.3f ms\n", hops, inet_ntoa(source_add_IP), (float)(end - start) / CLOCKS_PER_SEC * 1000);
                        }
                        close(sockfd_icmp);
                        close(sockfd_udp);
                        return 0;
                    }
                }
                // Ignore the packet and go back to select()
                else
                {
                    timeout = end - start;
                    valid = 0;
                    if (timeout >= 0.01)
                        continue;
                    else
                    {
                        // Number of repeats exceeded
                        if (noOfTransmissions > 3)
                        {
                            printf("   %d\t\t*\t\t*\n", hops);
                            noOfTransmissions = 1;
                            hops++;
                        }
                        timeout = TIMEOUT;
                        valid = 1;
                        continue;
                    }
                }
            }
        }
    }
    // Close the sockets
    close(sockfd_udp);
    close(sockfd_icmp);
    return 0;
}