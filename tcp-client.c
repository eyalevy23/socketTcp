/*
     TCP/IP client
*/

#include <stdio.h>

#if defined _WIN32

// link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>

#else
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#define SERVER_PORT 5060
#define SERVER_IP_ADDRESS "127.0.0.1"

void printOnComplete(int bytesSent, int messageLen);
void sendExit(int sock);
int validateAuthentication(int authentication);
int createSocket();

struct fileResult
{
     int fileSize;
     char *fileContent;
};

char *readFile(int part, long *fileSize)
{

     FILE *f = fopen("gistfile1.txt", "rb");
     fseek(f, 0, SEEK_END);
     long fsize = ftell(f);
     fseek(f, part ? fsize / 2 : 0, SEEK_SET); /* same as rewind(f); */

     char *string = malloc(fsize / 2 + 1);
     fread(string, fsize / 2, 1, f);
     fclose(f);
     *fileSize = fsize;
     return string;
}

int main()
{
     int bytesSent;
     long messageLen = 0;
     char *fileContent1 = readFile(0, &messageLen);
     char *fileContent2 = readFile(1, &messageLen);
     // printf("%s\n",fileContent1 );
     // printf("%s\n",fileContent2 );
     int sock = createSocket();
     // Sends some data to server
     printf("reading file\n");

     // int messageLen = massegeLenf();
     int total = 0;
     bytesSent = send(sock, fileContent1, messageLen / 2, 0);
     total += bytesSent;

     if (validateAuthentication(sock) == -1)
     {
          return -1;
     }

     // Change the CC Algorithm
     // Send the second part
     bytesSent = send(sock, fileContent2, messageLen / 2, 0);
     // sleep(1);
     total += bytesSent;
     printOnComplete(total, messageLen);
     sendExit(sock);
     // TODO: All open clientSocket descriptors should be kept
     // in some container and closed as well.
     close(sock);
     return 0;
}

int createSocket()
{
     int sock = socket(AF_INET, SOCK_STREAM, 0);

     if (sock == -1)
     {
          printf("Could not create socket : %d", errno);
     }

     // "sockaddr_in" is the "derived" from sockaddr structure
     // used for IPv4 communication. For IPv6, use sockaddr_in6
     //
     struct sockaddr_in serverAddress;
     memset(&serverAddress, 0, sizeof(serverAddress));

     serverAddress.sin_family = AF_INET;
     serverAddress.sin_port = htons(SERVER_PORT);
     int rval = inet_pton(AF_INET, (const char *)SERVER_IP_ADDRESS, &serverAddress.sin_addr);
     if (rval <= 0)
     {
          printf("inet_pton() failed");
          return -1;
     }

     // Make a connection to the server with socket SendingSocket.

     if (connect(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
     {
          printf("connect() failed with error code : %d", errno);
     }

     printf("connected to server\n");
     return sock;
}

void printOnComplete(int bytesSent, int messageLen)
{
     if (-1 == bytesSent)
     {
          printf("send() failed with error code : %d", errno);
     }
     else if (0 == bytesSent)
     {
          printf("peer has closed the TCP connection prior to send().\n");
     }
     else if (messageLen > bytesSent)
     {
          printf("sent only %d bytes from the required %d.\n", messageLen, bytesSent);
     }
     else
     {
          printf("message was successfully sent .\n");
     }
}

int validateAuthentication(int sock)
{
     int authentication;
     int byteRecieved = recv(sock, &authentication, 4, 0);
     if (authentication == (5512 ^ 6035))
     {
          printf("authentication confirmed\n");
          return 1;
     }
     else
     {
          printf("authentication failed");
          return -1;
     }
}

void sendExit(int sock)
{
     char exitMessege[] = "exit";
     int messageLen2 = strlen(exitMessege) + 1;
     send(sock, exitMessege, 1024, 0);
}
