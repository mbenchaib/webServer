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
    inet_pton(PF_INET, "10.12.7.3", &server.sin_addr);

    if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        perror("bind");
        return 1;
    }

    // listen
    if (listen(sockfd, 5) == -1)
    {
        perror("listen");
        return 1;
    }

    printf("Server waiting on port 8080...\n");

    // accept
    int client_fd = accept(sockfd, NULL, NULL);
    if (client_fd == -1)
    {
        perror("accept");
        return 1;
    }

    // receive
    char buffer[1024];
    int bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0)
    {
        buffer[bytes] = '\0';
        printf("Received: %s\n", buffer);
    }

    // reply
    char *msg = "Hello from server";
    send(client_fd, msg, strlen(msg), 0);

    close(client_fd);
    close(sockfd);

}