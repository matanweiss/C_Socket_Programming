#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#define PORT 9999
#define IP_ADDRESS "127.0.0.1"
#define BUFFER_SIZE 2048
#define FILE_NAME "file.txt"
#define ID1 2264
#define ID2 8585

int main()
{
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

    // creating the socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("socket() failed");
        return -1;
    }

    struct sockaddr_in Address;
    memset(&Address, 0, sizeof(Address));

    Address.sin_family = AF_INET;
    Address.sin_port = htons(PORT);
    int inetResult = inet_pton(AF_INET, (const char *)IP_ADDRESS, &Address.sin_addr);
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

    char choise = 'y';
    while (choise == 'y')
    {

        // setting cc algorithm to default value
        if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, "reno", 4) != 0)
        {
            perror("setsockopt() failed");
            return 1;
        }

        // sending the first part of the file
        char data[halfSize];
        fread(data, 1, halfSize, fp);
        if (send(sock, data, sizeof(data), 0) == -1)
        {
            perror("send() failed");
            return -1;
        }
        bzero(data, halfSize);
        long endOfFirstHalf = ftell(fp);
        printf("sent file of size %ld\n", endOfFirstHalf);

        // authenticating

        int authentication = ID1 ^ ID2;
        int receiverAuthentication;
        if (recv(sock, &receiverAuthentication, sizeof(int), 0) <= 0)
        {
            perror("recv() failed");
            return -1;
        }
        if (authentication == receiverAuthentication)
        {
            printf("authentication succeeded \n");
        }
        else
        {
            printf("authentication failed \n");
            close(sock);
            fclose(fp);
            return -1;
        }

        // changing cc algorithm
        if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, "cubic", 5) != 0)
        {
            perror("setsockopt() failed");
            return 1;
        }

        // sending the second part
        fread(data, 1, halfSize, fp);
        if (send(sock, data, sizeof(data), 0) == -1)
        {
            perror("send() failed");
            return -1;
        }
        bzero(data, BUFFER_SIZE);
        printf("sent file of size %ld\n", ftell(fp) - endOfFirstHalf);

        // returning the file position to the beginning of the file
        rewind(fp);

        printf("send the file again? y/n\n");
        scanf(" %c", &choise);
    }

    // sending an exit message to the receiver
    char *message = "exit";
    if (send(sock, message, sizeof(message), 0) == -1)
    {
        perror("send() failed");
        return -1;
    }

    close(sock);
    fclose(fp);
    return 0;
}
