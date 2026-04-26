#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/_endian.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main()
{
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf("Error\n");
        return (1);
    }
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));

    server.sin_family = PF_INET;
    server.sin_port = htons(8080);
    inet_pton(PF_INET, "127.0.0.1", &server.sin_addr);

    int result = connect(sockfd, (struct sockaddr *)&server, sizeof(server));
    if (result == -1)
    {
        perror("connect fail");
        return (1);
    }
    send(sockfd, "Hello world", 11, 0);
    close(sockfd);
}