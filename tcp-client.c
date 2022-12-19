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


#define SERVER_PORT 5060
#define SERVER_IP_ADDRESS "127.0.0.1"
#define MAX 4096

void printOnComplete(int bytesSent, int messageLen);
void sendExit(int sock);
int validateAuthentication(int authentication);
int createSocket();
long send_file(FILE *fp, int sockfd, long messegeLen);
void sendCompleteHalf(int sock);

FILE *readFile2(long *fileSize)
{
     FILE *f = fopen("gistfile1.txt", "rb");
     fseek(f, 0, SEEK_END);
     long fsize = ftell(f);
     long mid = fsize/2;
     rewind(f);
     *fileSize = fsize;
     return f;
}

// char *readFile(int part, long *fileSize)
// {

//      FILE *f = fopen("textfile.txt", "rb");
//      fseek(f, 0, SEEK_END);
//      long fsize = ftell(f);
//      fseek(f, part ? fsize / 2 : 0, SEEK_SET); /* same as rewind(f); */

//      char *string = malloc(fsize / 2 + 1);
//      fread(string, fsize / 2, 1, f);
//      fclose(f);
//      *fileSize = fsize;
//      return string;
// }



int main()
{

     int bytesSent;
     long messageLen = 0;
     //read file 
     printf("reading file\n");
     FILE *fp = readFile2(&messageLen);

     //create socket 
     int sock = createSocket();


     char cubic[10] = {0};
     strcpy(cubic, "cubic");
     int hasSentAuth = 0;
     int loop = 1;
     while (loop == 1)
     {    
          rewind(fp);
          // CC Algorithm : cubic
          // setsockopt(sock,IPPROTO_TCP,TCP_CONGESTION,cubic,5);
          // bzero(cubic, 10);
          // getsockopt(sock,IPPROTO_TCP,TCP_CONGESTION,&cubic,5);
          // printf("%s", cubic);

          int total = 0;
          //send first part of file
          long bytesSent = send_file(fp, sock, messageLen/2);
          sendCompleteHalf(sock);
          // bytesSent = send(sock, fileContent1, messageLen / 2, 0);
          // send_file(fileContent1,sock);
          total += bytesSent;

          if (hasSentAuth == 0){
               if (validateAuthentication(sock) == -1)
               {
                    return -1;
               }
               hasSentAuth = 1;
          }

          // Change the CC Algorithm to reno
          setsockopt(sock,IPPROTO_TCP,TCP_CONGESTION,"reno",4);

          // Send the second part of file 
          send_file(fp, sock, messageLen/2);
          // bytesSent = send(sock, fileContent2, messageLen / 2, 0);

          total += bytesSent;
          printOnComplete(total, messageLen);
          printf("enter 1 to continue : ");
          scanf("%d", &loop);
     }
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
     char exitMessege[] = "exit";
     int messageLen2 = strlen(exitMessege) + 1;
     send(sock, exitMessege, 1024, 0);
}

void sendCompleteHalf(int sock)
{
     char compMessage[] = "half";
     int messageLen2 = strlen(compMessage) + 1;
     send(sock, compMessage, 1024, 0);
}



long send_file(FILE *fp, int sockfd, long messegeLen){
     int bytesToRead = MAX;
     if(messegeLen<MAX){
          bytesToRead = messegeLen;
     }
     char data[MAX] = {0};
     
     int totalRead = 0;

     while( totalRead < messegeLen && fread(data, bytesToRead,1 ,fp) > 0) {
          int dataSend = send(sockfd, data, sizeof(data), 0); 
          if (dataSend == -1) {
               perror("[-]Error in sending file.");
               exit(1);
          }
          totalRead += bytesToRead;
          bzero(data, bytesToRead);
     }
     return totalRead;
}

/*unneeded code :
     char *fileContent1 = readFile(0, &messageLen);
     char *fileContent2 = readFile(1, &messageLen);
     fseek(f, part ? fsize / 2 : 0, SEEK_SET); /* same as rewind(f);
          int fileBytesSend = send(sock, &messageLen, sizeof(messageLen), 0);
     printf("messege len sent\n");
*/

