#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#define SERVER_PORT 5060
#define SERVER_IP_ADDRESS "127.0.0.1"
#define BUFFER_SIZE 2048
#define FILE_NAME "file.txt"

int main()
{
    char choise = 'y';
    while (choise == 'y')
    {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1)
        {
            perror("socket() failed");
            return -1;
        }

        // "sockaddr_in" is the "derived" from sockaddr structure
        // used for IPv4 communication. For IPv6, use sockaddr_in6
        //
        struct sockaddr_in Address;
        memset(&Address, 0, sizeof(Address));

        Address.sin_family = AF_INET;
        Address.sin_port = htons(SERVER_PORT);                                                   // (5001 = 0x89 0x13) little endian => (0x13 0x89) network endian (big endian)
        int inetResult = inet_pton(AF_INET, (const char *)SERVER_IP_ADDRESS, &Address.sin_addr); // convert IPv4 and IPv6 addresses from text to binary form
        // e.g. 127.0.0.1 => 0x7f000001 => 01111111.00000000.00000000.00000001 => 2130706433
        if (inetResult <= 0)
        {
            perror("inet_pton() failed");
            return -1;
        }

        // connect to receiver
        int connectResult = connect(sock, (struct sockaddr *)&Address, sizeof(Address));
        if (connectResult == -1)
        {
            perror("connect() failed");
            close(sock);
            return -1;
        }

        printf("connected to receiver\n");

        // opening the file
        FILE *fp;
        fp = fopen(FILE_NAME, "r");
        if (fp == NULL)
        {
            perror("fopen() failed");
            return -1;
        }

        // getting the file size
        fseek(fp, 0L, SEEK_END);
        long fileSize = ftell(fp);
        long halfSize = fileSize / 2;
        rewind(fp);

        // sending the first part
        char data[BUFFER_SIZE] = {0};
        while (fgets(data, BUFFER_SIZE, fp) != NULL && ftell(fp) < halfSize)
        {
            if (send(sock, data, sizeof(data), 0) == -1)
            {
                perror("send() failed");
                return -1;
            }
            bzero(data, BUFFER_SIZE);
        }

        long endOfFirstHalf = ftell(fp);
        printf("sent file of size %ld\n", endOfFirstHalf);

        // authenticating
        int id1 = 2264;
        int id2 = 8585;
        int authentication = id1 ^ id2;
        int serverAuthentication;
        if (recv(sock, &serverAuthentication, sizeof(int), 0) <= 0)
        {
            perror("recv() failed");
            return -1;
        }
        if (authentication == serverAuthentication)
        {
            printf("authentication succeeded \n");
        }
        else
        {
            printf("authentication failed \n");
        }

        // changing cc algorithm
        if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, "reno", 4) != 0)
        {
            perror("setsockopt() failed");
            return 1;
        }

        // sending the second part
        while (fgets(data, BUFFER_SIZE, fp) != NULL)
        {
            if (send(sock, data, sizeof(data), 0) == -1)
            {
                perror("send() failed");
                return -1;
            }
            bzero(data, BUFFER_SIZE);
        }

        printf("sent file of size %ld\n", ftell(fp) - endOfFirstHalf);

        fclose(fp);
        fp = NULL;

        printf("send the file again? y/n\n");
        scanf(" %c", &choise);
        close(sock);
    }
    return 0;
}
