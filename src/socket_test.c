#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>

#define MYPORT  88
#define BUFFER_SIZE 2048

int hexToBin(char ch) {
    if ('0' <= ch && ch <= '9') {
        return ch - '0';
    }
    if ('A' <= ch && ch <= 'F') {
        return ch - 'A' + 10;
    }
    if ('a' <= ch && ch <= 'f') {
        return ch - 'a' + 10;
    }
    return -1;
}

int main()
{
    ///定义sockfd
    int sock_cli = socket(AF_INET,SOCK_STREAM, 0);

    ///定义sockaddr_in
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(MYPORT);  ///服务器端口
    struct hostent *host;
    host = gethostbyname("livecmt-1.bilibili.com");
    for(int i=0; host->h_addr_list[i]; i++){
            printf("IP addr %d: %s\n", i+1, inet_ntoa( *(struct in_addr*)host->h_addr_list[i] ) );
        }

    servaddr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)host->h_addr_list[0]));  ///服务器ip

    ///连接服务器，成功返回0，错误返回-1
    if (connect(sock_cli, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("connect");
        exit(1);
    }

    char sendbuf[BUFFER_SIZE];
    char recvbuf[BUFFER_SIZE];

    /* char s[64]; */
    /* sprintf(s, "0101000c%08X%08X", 5269, 0); */
    char *s = "0101000c0000149500000000";
    int i;
    int j;
    printf("strlen: %lu\n", strlen(s));
    char fs[strlen(s)/2];
    while(i < strlen(s))
    {
        printf("%d\n", i);
        int h = hexToBin(s[i++]);
        int l = hexToBin(s[i++]);
        if (h == -1 || l == -1){
            printf("ERROR!!!\n");
            break;
        }
        fs[j] = (char)(h * 16 + l);
        printf("%c\n", fs[j]);
        j++;
    }
    printf("Start send\n");
    printf("%s\n", fs);
    send(sock_cli, fs, 12,0); ///发送
    printf("end send\n");

    /* while (fgets(sendbuf, sizeof(sendbuf), stdin) != NULL) */
    while (1)
    {
        /* send(sock_cli, sendbuf, strlen(sendbuf),0); ///发送 */
        /* if(strcmp(sendbuf,"exit\n")==0) */
            /* break; */
        int n = recv(sock_cli, recvbuf, sizeof(recvbuf),0); ///接收
        /* int n = read(sock_cli, recvbuf, sizeof(recvbuf)-1); ///接收 */
        printf("%d\n", n);
        /* fputs(recvbuf, stdout); */
        /* printf("%s\n", recvbuf); */
        for (int i = 0; i < n; i++){
            printf("%c", recvbuf[i]);
        }
        printf("\n");


        /* memset(sendbuf, 0, sizeof(sendbuf)); */
        memset(recvbuf, 0, sizeof(recvbuf));
    }

    close(sock_cli);
    return 0;
}
