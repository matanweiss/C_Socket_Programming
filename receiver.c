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

    // accept incoming connection
    printf("Waiting for connections...\n");
    struct sockaddr_in senderAddress; //
    socklen_t senderAddressLen = sizeof(senderAddress);

    while (1)
    {
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

        FILE *fp = fopen(FILE_NAME, "w");
        char buffer[BUFFER_SIZE];
        long n = 0;
        while ((n += recv(senderSocket, buffer, BUFFER_SIZE, 0)) > 0 && HALF_FILE_SIZE > n)
        {

            // fwrite(buffer, sizeof(char), BUFFER_SIZE, fp);
            fprintf(fp, "%s", buffer);
            bzero(buffer, BUFFER_SIZE);
        }
        // fseek(fp, 0L, SEEK_END);

        long endOfFirstHalf = ftell(fp);
        printf("received file of size %ld\n", endOfFirstHalf);

        fclose(fp);

        int id1 = 2264;
        int id2 = 8585;
        int authentication = id1 ^ id2;
        if (send(senderSocket, &authentication, sizeof(authentication), 0) == -1)
        {
            perror("send() failed");
            return -1;
        }
        bzero(buffer, BUFFER_SIZE);

        fopen(FILE_NAME, "a");

        while (recv(senderSocket, buffer, BUFFER_SIZE, 0) > 0)
        {

            // fwrite(buffer, sizeof(char), BUFFER_SIZE, fp);
            fprintf(fp, "%s", buffer);
            bzero(buffer, BUFFER_SIZE);
        }
        // fseek(fp, 0L, SEEK_END);
        printf("received file of size %ld\n", ftell(fp) - endOfFirstHalf);
    }

    close(socketObject);

    return 0;
}
