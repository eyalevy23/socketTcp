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
#include <sys/time.h>
#include <time.h>
#include <stdbool.h>
#include <netinet/tcp.h>

#define TIMEOUT 300000
#define MAX 4096
#define SERVER_PORT 9999 // The port that the server listens
// code forom receiver to sender
#define EXITCODE "exit"
#define HALFCODE "half"
#define FINISHCODE "finish_all"

int reachedHalf(char buff[]);
int requestToExit(char buff[]);
int finishAll(char buff[]);
void sendAuthentication(int clientSocket);
void printOutTimes(double timing[], double timeFirstHalf, double timeLastPart, int counter);
void sendACK(int sock);
int recvACK(int sock);
// void setTime(double timing[], double begin, double end);

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
        double timing[100];
        long seconds, microseconds;
        int byteRecieved = 0;
        int continueToListenOnOpenSocket = 1;
        long totalRecieved = 0;
        int charRead = 0;
        double timeFirstHalf = 0;
        double timeLastPart = 0;
        int counter = 0;
        struct timeval start, end;
        bool startTiming = false;
        bool userDesitionTimeOut = false;
        char buff[MAX];
        long allFileRece;

        // CC Algorithm : cubic
        if (setsockopt(clientSocket, IPPROTO_TCP, TCP_CONGESTION, "cubic", 5) != 0)
        {
            perror("setsockopt field error");
            return -1;
        }
        while (continueToListenOnOpenSocket)
        {

            // the user have 2 minetes to answer if he want to send again
            if (userDesitionTimeOut)
            {
                struct timeval tv;
                tv.tv_sec = 120;
                tv.tv_usec = 0;
                setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv);
                userDesitionTimeOut = false;
            }

            // receiv file from clint
            int bytesRecv = recv(clientSocket, buff, MAX, 0);

            // start measure time for the first half
            if (!startTiming)
            {
                gettimeofday(&start, 0);
                startTiming = true;
            }

            charRead = strlen(buff);
            totalRecieved += charRead;

            if (reachedHalf(buff))
            {
                // stop time fot first half
                gettimeofday(&end, 0);
                seconds = (double)(end.tv_sec - start.tv_sec);
                microseconds = (double)(end.tv_usec - start.tv_usec);
                double temp = seconds + (microseconds * 1e-6);
                timing[counter] = temp;
                timeFirstHalf += temp;
                startTiming = false;
                counter++;

                sendACK(clientSocket); // send ack for confirmation that Receiver get the first half

                printf("first half recived. Total read: %ld\n", totalRecieved);

                // send Authentication key to clint
                printf("sendAuthentication\n");
                sendAuthentication(clientSocket);

                // Change the CC Algorithm : reno
                if (setsockopt(clientSocket, IPPROTO_TCP, TCP_CONGESTION, "reno", 4) != 0)
                {
                    perror("setsockopt field error");
                    return -1;
                }

                // measure time for second part
                if (!startTiming)
                {
                    gettimeofday(&start, 0);
                    startTiming = true;
                }
            }

            else if (finishAll(buff))
            {
                sendACK(clientSocket);
                printf("recive complete all. Total read: %ld\n", totalRecieved);
                // stop time for second part
                gettimeofday(&end, 0);
                seconds = (double)(end.tv_sec - start.tv_sec);
                microseconds = (double)(end.tv_usec - start.tv_usec);
                double temp = seconds + (microseconds * 1e-6);
                timing[counter] = temp;
                timeLastPart += temp;
                startTiming = false;
                counter++;
                allFileRece += totalRecieved;
                totalRecieved = 0;
                userDesitionTimeOut = true;
            }

            else if (requestToExit(buff))
            {
                printf("request to exit recived\n");
                break;
            }

            bzero(buff, sizeof(buff)); // reset buffer
        }
        printOutTimes(timing, timeFirstHalf, timeLastPart, counter);
        printf("average file received : %ld \n", allFileRece / (counter / 2));

    }

    // TODO: All open clientSocket descriptors should be kept
    // in some container and closed as well.

    close(listeningSocket);
    return 0;
}

void sendAuthentication(int clientSocket)
{
    int xor = (5512 ^ 3065);
    while (1)
    {
        int bytesSent = send(clientSocket, &xor, 4, 0);
        if (recvACK(clientSocket))
        {
            break;
        }
    }
}

int reachedHalf(char buff[])
{
    int result = strcmp(HALFCODE, buff);
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
    int result = strcmp(FINISHCODE, buff);
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
    int result = strcmp(EXITCODE, buff);
    if (result == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void printOutTimes(double timing[100], double timeFirstHalf, double timeLastPart, int counter)
{
    for (int i = 0; i < counter; i += 2)
    {
        printf("round %d : first part (seconds) : %f\n", (i / 2) + 1, timing[i]);
        printf("round %d : second part (seconds) : %f\n", (i / 2) + 1, timing[i + 1]);
    }

    printf("avarge time for first half : %f\n", (timeFirstHalf / (counter / 2)));
    printf("avarge time for last half : %f\n", (timeLastPart / (counter / 2)));
    printf("totle time : %f\n", (timeFirstHalf + timeLastPart));
}

int recvACK(int sock)//receive Ok from Sender
{
    char ACK[8];
    bzero(ACK, sizeof(ACK));
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = TIMEOUT;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv);
    recv(sock, ACK, sizeof(ACK), 0);
    int result = strcmp("OK", ACK);
    if (result == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void sendACK(int sock)//send Ok to Sender
{
    char compMessage[] = "OK";
    int messageLen2 = strlen(compMessage) + 1;
    int byteSend = send(sock, compMessage, 8, 0);
}
