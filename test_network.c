#include "network.h"
#include "message.pb-c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT 8080

int connect_to_server()
{
    printf("Attempting to connect to server %s:%d\n", SERVER_HOST, SERVER_PORT);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket");
        return -1;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_HOST, &serv_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(sockfd);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect");
        close(sockfd);
        return -1;
    }

    printf("Successfully connected to server, socket fd: %d\n", sockfd);
    return sockfd;
}

int main()
{
    printf("=== Network Test ===\n");

    // 서버에 연결
    int sockfd = connect_to_server();
    if (sockfd < 0)
    {
        printf("Failed to connect to server\n");
        return 1;
    }

    // Ping 메시지 생성 및 전송
    printf("Sending ping message...\n");
    ClientMessage ping_msg = CLIENT_MESSAGE__INIT;
    PingRequest ping_req = PING_REQUEST__INIT;
    ping_req.message = "Hello Server from test!";
    ping_msg.msg_case = CLIENT_MESSAGE__MSG_PING;
    ping_msg.ping = &ping_req;

    if (send_client_message(sockfd, &ping_msg) < 0)
    {
        printf("Failed to send ping message\n");
        close(sockfd);
        return 1;
    }

    printf("Ping message sent, waiting for response...\n");

    // 응답 수신
    ServerMessage *response = receive_server_message(sockfd);
    if (!response)
    {
        printf("Failed to receive server response\n");
        close(sockfd);
        return 1;
    }

    printf("Received response! Message type: %d\n", response->msg_case);

    if (response->msg_case == SERVER_MESSAGE__MSG_PING_RES)
    {
        printf("Ping response: %s\n", response->ping_res->message);
    }

    // 정리
    server_message__free_unpacked(response, NULL);
    close(sockfd);

    printf("Test completed successfully!\n");
    return 0;
}