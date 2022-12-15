/*
    TCP/IP-server
*/

#include <stdio.h>

// Linux and other UNIXes
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#define MAX 1024

#define SERVER_PORT 5060 // The port that the server listens

void readFromSocket(int clientSocket)
{
    char buff[MAX];
    int n;
    int numread = 1;
    while (numread > 0)
    {
        // read the message from client and copy it in buffer
        numread = read(clientSocket, buff, sizeof(buff));
        // print buffer which contains the client contents
        if (numread > 0)
        {
            printf("%s", buff);
        }
        else
        {
            printf("\n completed reading file \n");
        }
    }
}

int main()
{

    // Open the listening (server) socket
    int listeningSocket = -1;

    if ((listeningSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("Could not create listening socket : %d", errno);
    }

    // Reuse the address if the server socket on was closed
    // and remains for 45 seconds in TIME-WAIT state till the final removal.
    //
    int enableReuse = 1;
    if (setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0)
    {
        printf("setsockopt() failed with error code : %d", errno);
    }

    // "sockaddr_in" is the "derived" from sockaddr structure
    // used for IPv4 communication. For IPv6, use sockaddr_in6
    //
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(SERVER_PORT); // network order

    // Bind the socket to the port with any IP at this port
    if (bind(listeningSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        printf("Bind failed with error code : %d", errno);
        // TODO: close the socket
        return -1;
    }

    printf("Bind() success\n");

    // Make the socket listening; actually mother of all client sockets.
    if (listen(listeningSocket, 500) == -1) // 500 is a Maximum size of queue connection requests
                                            // number of concurrent connections
    {
        printf("listen() failed with error code : %d", errno);
        // TODO: close the socket
        return -1;
    }

    // Accept and incoming connection
    printf("Waiting for incoming TCP-connections...\n");

    struct sockaddr_in clientAddress; //
    socklen_t clientAddressLen = sizeof(clientAddress);
    int continueToListnerToNewConnections = 1;
    while (continueToListnerToNewConnections)
    {
        memset(&clientAddress, 0, sizeof(clientAddress));
        clientAddressLen = sizeof(clientAddress);
        int clientSocket = accept(listeningSocket, (struct sockaddr *)&clientAddress, &clientAddressLen);
        printf("A new client connection accepted\n");
        
        if (clientSocket == -1)
        {
            printf("listen failed with error code : %d", errno);
            // TODO: close the sockets
            return -1;
        }
        int continueToListenOnOpenSocket = 1;
        int hasSentAuth = 0;
        while (continueToListenOnOpenSocket)
        {
            char buff[MAX];
            // frist file recive
            int byteRecieved = recv(clientSocket, buff, MAX, 0);
            int result = strcmp("exit", buff);
            if (result == 0 || byteRecieved == 0)
            {
                printf("closing client connection. Recieved exit \n");
                break;
            }
            // readFromSocket(clientSocket);
            // send authentication
            if (hasSentAuth == 0)
            {
                int xor = (5512 ^ 6035);
                int bytesSent = send(clientSocket, &xor, 4, 0);
                hasSentAuth = 1;
            }
        }

        // Reply to client
        //  char message[] = "Welcome to our TCP-server\n";
        //  int messageLen = strlen(message) + 1;

        // int bytesSent = send(clientSocket, message, messageLen, 0);
        // if (-1 == bytesSent)
        // {
        // 	printf("send() failed with error code : %d" , errno);
        // }
        // else if (0 == bytesSent)
        // {
        //    printf("peer has closed the TCP connection prior to send().\n");
        // }
        // else if (messageLen > bytesSent)
        // {
        //    printf("sent only %d bytes from the required %d.\n", bytesSent, messageLen);
        // }
        // else
        // {
        //    printf("message was successfully sent .\n");
        // }
    }

    // TODO: All open clientSocket descriptors should be kept
    // in some container and closed as well.

    close(listeningSocket);
    return 0;
}
