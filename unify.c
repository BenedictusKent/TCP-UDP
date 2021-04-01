#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h> 

#define ERR_EXIT(m) \
        do \
        { \
                perror(m); \
                exit(EXIT_FAILURE); \
        } while(0)

void echo_cli(int sock, FILE* fp, char* filename)
{
    struct stat stats;
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(5188);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int ret;
    char sendbuf[1024] = {0};
    char recvbuf[1024] = {0};

    // file size
    float size = 0;
    if(stat(filename, &stats) == 0)
        size = (float) stats.st_size;
    else
        printf("Error reading file\n");

    // send file
    int current = 0, count = 1;
    int arr[3] = {25, 50, 75};
    clock_t start, end;
    double cputime;
    start = clock();
    float temp = size/4;
    while(fgets(sendbuf,1024,fp) != NULL) 
    {
        if(current == 0) 
        {
            time_t t = time(NULL);
            struct tm tm = *localtime(&t);
            printf("0%% ");
            printf("%d/%02d/%02d ", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday);
            printf("%02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
        }
        else 
        {
            if((current >= (int)temp*count) && count < 4) 
            {
                time_t t = time(NULL);
                struct tm tm = *localtime(&t);
                printf("%d%% ", arr[count-1]);
                printf("%d/%02d/%02d ", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday);
                printf("%02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
                count += 1;
            }
        }
        sendto(sock, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
        ret = recvfrom(sock, recvbuf, sizeof(recvbuf), 0, NULL, NULL);
        if (ret == -1)
        {
            if (errno == EINTR)
                continue;
            ERR_EXIT("recvfrom");
        }
        current += (int) strlen(sendbuf);
        memset(sendbuf, 0, sizeof(sendbuf));
        memset(recvbuf, 0, sizeof(recvbuf));
    }
    end = clock();

    // infomational
    cputime = ((double) (end-start)) / 1000;
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    printf("100%% ");
    printf("%d/%02d/%02d ", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday);
    printf("%02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
    printf("Total trans time: %d ms\n", (int)cputime);
    temp = size;
    if(size < 1000)
    {
        printf("File size: %d B\n", (int) size);
    }
    else if(size >= 1000 && size < 1000000)
    {
        size /= 1000;
        printf("File size: %.1f KB\n", size);
    }
    else
    {
        size /= 1000000;
        printf("File size: %.1f MB\n", size);
    }

    // file size
    float sent = 0;
    if(stat("receive.txt", &stats) == 0)
        sent = (float) stats.st_size;
    else
        printf("Error reading file\n");

    printf("Packet loss: %.2f%%\n", ((temp-sent)/temp)*100);
    printf("File sent successfully\n");

    close(sock);
}

void echo_ser(int sock)
{
    char recvbuf[1024] = {0};
    struct sockaddr_in peeraddr;
    socklen_t peerlen;
    int n;
    FILE *fp;

    // check if 'receive.txt' exist
    fp = fopen("receive.txt", "r");
    if(fp != NULL)
    {
        fclose(fp);
        remove("receive.txt");      // delete if exists
    }

    while (1)
    {
        peerlen = sizeof(peeraddr);
        memset(recvbuf, 0, sizeof(recvbuf));
        n = recvfrom(sock, recvbuf, sizeof(recvbuf), 0,
                     (struct sockaddr *)&peeraddr, &peerlen);
        if (n == -1)
        {
            if (errno == EINTR)
                continue;

            ERR_EXIT("recvfrom error");
        }
        else if(n > 0)
        {
            fp = fopen("receive.txt", "a");
            fprintf(fp, "%s", recvbuf);
            sendto(sock, recvbuf, n, 0,
                   (struct sockaddr *)&peeraddr, peerlen);
            fclose(fp);
        }
    }
    close(sock);
}

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    // var declaration
    int sockfd, newsockfd, portno, n, sock;
    socklen_t clilen;
    char buffer[1024], *task, *mode;
    struct sockaddr_in serv_addr, cli_addr;
    struct hostent *server;
    struct stat stats;
    FILE *fp;
    char *filename;

    mode = argv[1];
    task = argv[2];
    if(strcmp(mode, "tcp") == 0)        // tcp
    {
        if(strcmp(task, "send") == 0)           // client
        {
            // check error input
            if(argc < 6)
            {
                fprintf(stderr, "usage %s tcp send <ip> <port> <filename>\n", argv[0]);
                exit(0);
            }

            // open socket
            portno = atoi(argv[4]);
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) error("ERROR opening socket");

            // open server
            server = gethostbyname(argv[3]);
            if(server == NULL)
            {
                fprintf(stderr, "ERROR, no such host\n");
                exit(0);
            }

            // setup
            bzero((char *) &serv_addr, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            bcopy(  (char *)server->h_addr, 
                    (char *)&serv_addr.sin_addr.s_addr,
                    server->h_length);
            serv_addr.sin_port = htons(portno);
            if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
                error("ERROR connecting");

            // open file
            filename = argv[5];
            fp = fopen(filename, "r");
            if(fp == NULL) 
            {
                error("Error reading file\n");
                exit(0);
            }

            // file size
            float size = 0;
            if(stat(filename, &stats) == 0)
                size = (float) stats.st_size;
            else
                printf("Error reading file\n");

            // send file
            int current = 0, count = 1;
            int arr[3] = {25, 50, 75};
            clock_t start, end;
            double cputime;
            bzero(buffer,1024);
            float temp = size/4;
            start = clock();
            while(fgets(buffer,1024,fp) != NULL) 
            {
                if(current == 0) 
                {
                    time_t t = time(NULL);
                    struct tm tm = *localtime(&t);
                    printf("0%% ");
                    printf("%d/%02d/%02d ", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday);
                    printf("%02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
                }
                else 
                {
                    if((current >= (int)temp*count) && count < 4) 
                    {
                        time_t t = time(NULL);
                        struct tm tm = *localtime(&t);
                        printf("%d%% ", arr[count-1]);
                        printf("%d/%02d/%02d ", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday);
                        printf("%02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
                        count += 1;
                    }
                }
                n = send(sockfd, buffer, sizeof(buffer), 0);
                current += (int)strlen(buffer);
                bzero(buffer,1024);
            }
            end = clock();

            // infomational
            cputime = ((double) (end-start)) / 1000;
            time_t t = time(NULL);
            struct tm tm = *localtime(&t);
            printf("100%% ");
            printf("%d/%02d/%02d ", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday);
            printf("%02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
            printf("Total trans time: %d ms\n", (int)cputime);
            if(size < 1000)
            {
                printf("File size: %d B\n", (int) size);
            }
            else if(size >= 1000 && size < 1000000)
            {
                size /= 1000;
                printf("File size: %.1f KB\n", size);
            }
            else
            {
                size /= 1000000;
                printf("File size: %.1f MB\n", size);
            }   
            printf("File sent successfully\n");
            fclose(fp);
            close(sockfd);
        }
        else if(strcmp(task, "recv") == 0)       // server
        {
            // check error input
            if(argc < 5)
            {
                fprintf(stderr, "usage %s tcp recv <ip> <port>\n", argv[0]);
                exit(0);
            }

            // open socket
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0)
                error("ERROR opening socket");

            // setup
            bzero((char *) &serv_addr, sizeof(serv_addr));
            portno = atoi(argv[4]);
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_addr.s_addr = INADDR_ANY;
            serv_addr.sin_port = htons(portno);
            if (bind(sockfd, (struct sockaddr *) &serv_addr,
                    sizeof(serv_addr)) < 0)
                    error("ERROR on binding");
            listen(sockfd,5);
            clilen = sizeof(cli_addr);
            newsockfd = accept(sockfd,
                        (struct sockaddr *) &cli_addr,
                        &clilen);
            if (newsockfd < 0)
                error("ERROR on accept");

            // open file and receive data
            filename = "receive.txt";
            fp = fopen(filename, "w");
            // int line = 0, total = 0;
            while(1) 
            {
                n = recv(newsockfd, buffer, 1024, 0);
                if(n <= 0) break;
                fprintf(fp,"%s",buffer);
                bzero(buffer,1024);
            }
            fclose(fp);
            close(newsockfd);
            close(sockfd);

            // print file size
            float size = 0;
            if(stat(filename, &stats) == 0)
            {
                size = (float) stats.st_size;
                if(size < 1000)
                {
                    printf("File size: %d B\n", (int) size);
                }
                else if(size >= 1000 && size < 1000000)
                {
                    size /= 1000;
                    printf("File size: %.1f KB\n", size);
                }
                else
                {
                    size /= 1000000;
                    printf("File size: %.1f MB\n", size);
                }   
            }
            else
                printf("Error reading file\n");
            printf("Data written successfully\n");
        }
        else
        {
            fprintf(stderr,"usage %s <tcp/udp> <send/recv> <ip> <port> <filename>\n", argv[0]);
            exit(0);
        }
    }
    else if(strcmp(mode, "udp") == 0)   // udp
    {
        if(strcmp(task, "send") == 0)           // client
        {
            // check error input
            if(argc < 6)
            {
                fprintf(stderr, "usage %s udp send <ip> <port> <filename>\n", argv[0]);
                exit(0);
            }

            // setup
            if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
                ERR_EXIT("socket");

            // open file
            filename = argv[5];
            fp = fopen(filename, "r");
            if(fp == NULL) 
            {
                error("Error reading file\n");
                exit(0);
            }

            // start
            echo_cli(sock, fp, filename);
        }
        else if(strcmp(task, "recv") == 0)      // server
        {
            // check error input
            if(argc < 5)
            {
                fprintf(stderr, "usage %s udp recv <ip> <port>\n", argv[0]);
                exit(0);
            }

            // setup
            if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
                ERR_EXIT("socket error");
            struct sockaddr_in servaddr;
            memset(&servaddr, 0, sizeof(servaddr));
            servaddr.sin_family = AF_INET;
            servaddr.sin_port = htons(5188);
            servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
            if (bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
                ERR_EXIT("bind error");

            // start
            echo_ser(sock);
        }
        else
        {
            fprintf(stderr,"usage %s <tcp/udp> <send/recv> <ip> <port> <filename>\n", argv[0]);
            exit(0);
        }
    }
    else
    {
        fprintf(stderr,"usage %s <tcp/udp> <send/recv> <ip> <port> <filename>\n", argv[0]);
        exit(0);
    }
    
    return 0;
}
