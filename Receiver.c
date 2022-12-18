#include <stdio.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <sys/time.h>

#define CVECTOR_LOGARITHMIC_GROWTH
#include "cvector.h"

#define SERVER_PORT 5060
#define BUFFER_SIZE 2048
#define HALF_FILE_SIZE 1050426
#define ID1 2264
#define ID2 8585

void printTimes(long *times)
{
    if (times)
    {
        int count = 0;
        long firstSum = 0, secondSum = 0;
        for (size_t i = 0; i < cvector_size(times); i = i + 2)
        {
            count++;
            firstSum += times[i];
            printf("first part time of file %ld: %ld\n", (i / 2) + 1, times[i]);
            secondSum += times[i + 1];
            printf("second part time of file %ld: %ld\n", (i / 2) + 1, times[i + 1]);
        }
        printf("average time to send the first part: %ld\n", firstSum / count);
        printf("average time to send the second part: %ld\n", secondSum / count);
        cvector_free(times);
    }
}

int get_files(int senderSocket)
{

    cvector_vector_type(long) times = NULL;
    struct timeval start, end;

    while (1)
    {
        // setting cc algorithm to default value
        if (setsockopt(senderSocket, IPPROTO_TCP, TCP_CONGESTION, "cubic", 5) != 0)
        {
            perror("setsockopt() failed");
            return 1;
        }

        // receiving the first half
        gettimeofday(&start, NULL);
        char buffer[HALF_FILE_SIZE];
        bzero(buffer, HALF_FILE_SIZE);
        while (strcmp(buffer, "sent"))
        {
            if (recv(senderSocket, buffer, BUFFER_SIZE, 0) < 0)
            {
                perror("recv() failed");
                return -1;
            }
            if (!strcmp(buffer, "exit"))
            {
                printTimes(times);
                return 0;
            }
        }

        gettimeofday(&end, NULL);
        bzero(buffer, HALF_FILE_SIZE);
        printf("received the first half of the file\n");

        // measuring the time to it took to receive the first part
        long timeDelta = (end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec;
        cvector_push_back(times, timeDelta);

        // sending authentication
        int authentication = ID1 ^ ID2;
        if (send(senderSocket, &authentication, sizeof(authentication), 0) == -1)
        {
            perror("send() failed");
            return -1;
        }

        // changing cc algorithm
        if (setsockopt(senderSocket, IPPROTO_TCP, TCP_CONGESTION, "reno", 4) != 0)
        {
            perror("setsockopt() failed");
            return 1;
        }

        // receiving the second half
        gettimeofday(&start, NULL);
        while (strcmp(buffer, "sent"))
        {
            if (recv(senderSocket, buffer, BUFFER_SIZE, 0) < 0)
            {
                perror("recv() failed");
                return -1;
            }
            if (!strcmp(buffer, "exit"))
            {
                printTimes(times);
                return 0;
            }
        }

        // measuring the time to it took to receive the secont part
        gettimeofday(&end, NULL);
        timeDelta = (end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec;
        cvector_push_back(times, timeDelta);

        printf("received the second half of the file\n");
    }
}

int main()
{
    // signal(SIGPIPE, SIG_IGN);  // on linux to prevent crash on closing socket

    // Open the receiving socket
    int socketObject = socket(AF_INET, SOCK_STREAM, 0);
    if (socketObject == -1)
    {
        perror("socket() failed");
        return 1;
    }

    // "sockaddr_in" is the "derived" from sockaddr structure
    // used for IPv4 communication. For IPv6, use sockaddr_in6
    //
    struct sockaddr_in Address;
    memset(&Address, 0, sizeof(Address));

    Address.sin_family = AF_INET;
    Address.sin_addr.s_addr = INADDR_ANY;  // any IP at this port (Address to accept any incoming messages)
    Address.sin_port = htons(SERVER_PORT); // network order (makes byte order consistent)

    // Bind the socket to the port with any IP at this port
    int bindResult = bind(socketObject, (struct sockaddr *)&Address, sizeof(Address));
    if (bindResult == -1)
    {
        perror("bind() failed");
        close(socketObject);
        return -1;
    }

    // listen to receiver
    int listenResult = listen(socketObject, 3);
    if (listenResult == -1)
    {
        perror("listen() failed");
        close(socketObject);
        return -1;
    }

    while (1)
    {
        // accept incoming connection
        printf("Waiting for connections...\n");
        struct sockaddr_in senderAddress; //
        socklen_t senderAddressLen = sizeof(senderAddress);
        memset(&senderAddress, 0, sizeof(senderAddress));
        senderAddressLen = sizeof(senderAddress);
        int senderSocket = accept(socketObject, (struct sockaddr *)&senderAddress, &senderAddressLen);
        if (senderSocket == -1)
        {
            perror("accept() failed");
            close(socketObject);
            return -1;
        }

        printf("Sender connection accepted\n");

        if (get_files(senderSocket) == -1)
        {
            close(socketObject);
            return -1;
        }
    }

    close(socketObject);

    return 0;
}
