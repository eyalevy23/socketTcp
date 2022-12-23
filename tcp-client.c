/*
     TCP/IP client
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <netinet/in.h>

#define TIMEOUT 300000
#define SERVER_PORT 9999
#define SERVER_IP_ADDRESS "127.0.0.1"
#define MAX 4096
// code forom server to receiver
#define EXITCODE "exit"
#define HALFCODE "half"
#define FINISHCODE "finish_all"

void printOnComplete(int bytesSent, int messageLen);
void sendExit(int sock);
int validateAuthentication(int authentication);
int createSocket();
long send_file(FILE *fp, int sockfd, long messegeLen);
void sendCompleteHalf(int sock);
void sendCompleteLastPart(int sock);
FILE *readFile(long *fileSize);
int recvACK(int sock);
void sendACK(int sock);

int main()
{
     int bytesSent = 0;
     long messageLen = 0;

     // read file
     FILE *fp = readFile(&messageLen);
     printf("%ld", messageLen);

     // create socket
     int sock = createSocket();

     int loop = 1;
     while (loop == 1)
     {
          bytesSent = 0;
          rewind(fp);
          // CC Algorithm : cubic
          if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, "cubic", 5) != 0)
          {
               perror("setsockopt field error");
               return -1;
          }

          int total = 0;
          // send first part of file
          bytesSent = send_file(fp, sock, messageLen / 2);
          while (1)
          {
               sendCompleteHalf(sock);
               if (recvACK(sock))
               {
                    break;
               }
          }

          total += bytesSent;

          // check for authentication

          int xor = 0;
          recv(sock, &xor, sizeof(xor), 0);
          if (xor != 0 && validateAuthentication(xor) == -1)
          {
               return -1;
          }
          sendACK(sock);

          // Change the CC Algorithm : reno
          if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, "reno", 4) != 0)
          {
               perror("setsockopt field error");
               return -1;
          }

          // Send the second part of file
          bytesSent = send_file(fp, sock, messageLen / 2);
          while (1)
          // in case of loss pocket send over and over intail Receiver get the sendCompleteLastPart meesage
          {
               sendCompleteLastPart(sock);
               if (recvACK(sock)) // whait to get ack from the Receiver
               {
                    break;
               }
          }
          total += bytesSent;

          printOnComplete(total, messageLen);

          // user desision for sending the file again
          // enter 1 for YES for exit any other key
          printf("enter 1 to continue : ");
          scanf("%d", &loop);
     }

     // finsih while loop and send exit in order to close the socket
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
          printf("sent only %d bytes from the required %d.\n", bytesSent, messageLen);
     }
     else
     {
          printf("message was successfully sent .\n");
     }
}

int validateAuthentication(int authentication)
{
     if (authentication == (5512 ^ 3065))
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
     char exitMessege[] = EXITCODE;
     int messageLen2 = strlen(exitMessege) + 1;
     send(sock, exitMessege, 64, 0);
}

void sendCompleteHalf(int sock)
{
     char compMessage[] = HALFCODE;
     int messageLen2 = strlen(compMessage) + 1;
     int byteSend = send(sock, compMessage, 64, 0);
}

void sendCompleteLastPart(int sock)
{
     char compMessage[] = FINISHCODE;
     int messageLen2 = strlen(compMessage) + 1;
     send(sock, compMessage, 64, 0);
}

FILE *readFile(long *fileSize)
{
     FILE *f = fopen("gistfile1.txt", "rb");
     fseek(f, 0, SEEK_END);
     long fsize = ftell(f);
     rewind(f);
     *fileSize = fsize;
     return f;
}

long send_file(FILE *fp, int sockfd, long messegeLen)
{
     int bytesToRead = MAX;
     if (messegeLen < MAX) // in case the file smaller than buffer
     {
          bytesToRead = messegeLen;
     }
     char data[MAX] = {0};
     int totalRead = 0;
     int dataSend = 0;
     int freadbytes;
     do 
     {    
          freadbytes = fread(data, bytesToRead, 1, fp);
          dataSend = send(sockfd, data, sizeof(data), 0);
          if (dataSend == -1)
          {
               perror("[-]Error in sending file.");
               exit(1);
          }
          totalRead += bytesToRead;
          // totalRead += dataSend;
          bzero(data, bytesToRead);
     }while ((totalRead < messegeLen) && freadbytes > 0);
     
     return totalRead;
}

int recvACK(int sock) // receive Ok from Receiver
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

void sendACK(int sock) //send Ok to Receiver
{
     char compMessage[] = "OK";
     int messageLen2 = strlen(compMessage) + 1;
     int byteSend = send(sock, compMessage, 8, 0);
}
