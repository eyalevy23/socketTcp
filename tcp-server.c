/*
    TCP/IP-server
*/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#define MAX 4096
#define SERVER_PORT 5060 // The port that the server listens

int reachedHalf(char buff[]);
int requestToExit(char buff[]);
int finishAll(char buff[]);
void sendAuthentication(int clientSocket);
void printOutTimes(double timeFirstHalf,double timeLastPart,int counter);

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
    struct sockaddr_in clientAddress; //
    socklen_t clientAddressLen = sizeof(clientAddress);
    int continueToListnerToNewConnections = 1;

    while (continueToListnerToNewConnections)
    {
        printf("Waiting for incoming TCP-connections...\n");
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
        int byteRecieved = 0;
        long messegeLen = 0;
        int continueToListenOnOpenSocket = 1;
        long totalRecieved = 0;
        int charRead = 0;
        double timeFirstHalf = 0;
        double timeLastPart = 0;
        clock_t start, end;
        double cpu_time_used;
        int counter = 0;

        while (continueToListenOnOpenSocket)
        {   
            char buff[MAX];

            //start measure time for the first half
            start = clock();

            //receiv file from clint
            recv(clientSocket, buff, MAX, 0);
            charRead = strlen(buff);
            totalRecieved += charRead;

            if (reachedHalf(buff))
            {
                printf("first half recived. Total read: %ld\n",totalRecieved);
                //stop time 
                end = clock();
                cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
                timeFirstHalf += cpu_time_used;
                counter++;

                //send Authentication key to clint
                sendAuthentication(clientSocket);

                //measure time for last part
                start = clock();
                
            }

            if(finishAll(buff)){
                printf("recive complete all. Total read: %ld\n",totalRecieved);
                //stop time
                end = clock();
                cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
                timeLastPart += cpu_time_used;
                totalRecieved = 0;
            }

            if (requestToExit(buff))
            {
                printf("request to exit recived\n");
                break;
            }
        }
        printOutTimes(timeFirstHalf, timeLastPart, counter);
    }

    

    // TODO: All open clientSocket descriptors should be kept
    // in some container and closed as well.

    close(listeningSocket);
    return 0;
}

void sendAuthentication(int clientSocket){
    int xor = (5512 ^ 3065);
    int bytesSent = send(clientSocket, &xor, 4, 0);
}

int reachedHalf(char buff[])
{
    int result = strcmp("half", buff);
    if (result == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int finishAll(char buff[])
{
    int result = strcmp("finish_all", buff);
    if (result == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int requestToExit(char buff[])
{
    int result = strcmp("exit", buff);
    if (result == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void printOutTimes(double timeFirstHalf,double timeLastPart,int counter){
    printf("avarge time for first half : %f\n", (timeFirstHalf/counter));
    printf("avarge time for last half : %f\n", (timeLastPart/counter));
    printf("totle time : %f\n", (timeFirstHalf + timeLastPart));
}