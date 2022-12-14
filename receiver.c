#include <stdio.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_PORT 5060
#define BUFFER_SIZE 2048
#define HALF_FILE_SIZE 1050426
#define FILE_NAME "received.txt"

int get_file(int senderSocket)
{
    while (1)
    {

        // long fileSize;
        // if (recv(senderSocket, &fileSize, sizeof(fileSize), 0) < 0)
        // {
        //     perror("recv() failed");
        //     return -1;
        // }
        // printf("%ld", fileSize);
        // if (!fileSize)
        // {
        //     break;
        // }

        // FILE *fp = fopen(FILE_NAME, "w");

        // receiving the first half
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
                return 0;
            }
        }
        bzero(buffer, HALF_FILE_SIZE);

        printf("received the first half of the file\n");

        // sending authentication
        int id1 = 2264;
        int id2 = 8585;
        int authentication = id1 ^ id2;
        if (send(senderSocket, &authentication, sizeof(authentication), 0) == -1)
        {
            perror("send() failed");
            return -1;
        }

        // receiving the second half
        while (strcmp(buffer, "sent"))
        {
            if (recv(senderSocket, buffer, BUFFER_SIZE, 0) < 0)
            {
                perror("recv() failed");
                return -1;
            }
            if (!strcmp(buffer, "exit"))
            {
                return 0;
            }
        }
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

        if (get_file(senderSocket) == -1)
        {
            close(socketObject);
            return -1;
        }
    }

    close(socketObject);

    return 0;
}
