#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <protobuf-c/protobuf-c.h>
#include "message.pb-c.h"

#define BUFFER_SIZE 1024

// 모든 데이터를 전송할 때까지 반복하여 전송한다
ssize_t send_all(int sockfd, const void *buf, size_t len)
{
    size_t total = 0;
    const uint8_t *p = buf;
    while (total < len)
    {
        ssize_t sent = send(sockfd, p + total, len - total, 0);
        if (sent <= 0)
        {
            if (sent < 0 && errno == EINTR)
                continue;
            perror("send");
            return -1;
        }
        total += sent;
    }
    return total;
}

// 지정된 바이트 수만큼 수신할 때까지 반복하여 수신한다
ssize_t recv_all(int sockfd, void *buf, size_t len)
{
    size_t total = 0;
    uint8_t *p = buf;
    while (total < len)
    {
        ssize_t recvd = recv(sockfd, p + total, len - total, 0);
        if (recvd <= 0)
        {
            if (recvd < 0 && errno == EINTR)
                continue;
            return recvd;
        }
        total += recvd;
    }
    return total;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    const char *server_ip = argv[1];
    int port = atoi(argv[2]);
    if (port <= 0 || port > 65535)
    {
        fprintf(stderr, "Invalid port number.\n");
        exit(EXIT_FAILURE);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Connected to %s:%d\n", server_ip, port);

    char input[BUFFER_SIZE];
    while (1)
    {
        printf("> ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin))
            break;
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n')
        {
            input[len - 1] = '\0';
            len--;
        }
        // EchoData 및 Message 초기화
        EchoData echo_data = ECHO_DATA__INIT;
        echo_data.msg = input;
        Message msg = MESSAGE__INIT;
        msg.op = OP_CODE__ECHO_MSG;
        msg.data_case = MESSAGE__DATA_ECHO;
        msg.echo = &echo_data;

        // 직렬화 및 전송
        size_t packed_len = message__get_packed_size(&msg);
        uint8_t *sendbuf = malloc(4 + packed_len);
        if (!sendbuf)
        {
            perror("malloc");
            break;
        }
        uint32_t netlen = htonl((uint32_t)packed_len);
        memcpy(sendbuf, &netlen, 4);
        message__pack(&msg, sendbuf + 4);

        if (send_all(sockfd, sendbuf, 4 + packed_len) < 0)
        {
            free(sendbuf);
            break;
        }
        free(sendbuf);

        // 응답 수신: 길이 prefix
        uint8_t lenbuf[4];
        ssize_t r = recv_all(sockfd, lenbuf, 4);
        if (r <= 0)
        {
            printf("Connection closed by server.\n");
            break;
        }
        uint32_t resp_len;
        memcpy(&resp_len, lenbuf, 4);
        resp_len = ntohl(resp_len);

        uint8_t *resp_buf = malloc(resp_len);
        if (!resp_buf)
        {
            perror("malloc");
            break;
        }
        r = recv_all(sockfd, resp_buf, resp_len);
        if (r <= 0)
        {
            free(resp_buf);
            printf("Connection closed by server.\n");
            break;
        }

        // 역직렬화 및 출력
        Message *resp_msg = message__unpack(NULL, resp_len, resp_buf);
        free(resp_buf);
        if (!resp_msg)
        {
            fprintf(stderr, "Failed to unpack response message.\n");
            break;
        }
        if (resp_msg->op == OP_CODE__ECHO_MSG && resp_msg->data_case == MESSAGE__DATA_ECHO)
        {
            printf("Received: %s\n", resp_msg->echo->msg);
        }
        else
        {
            printf("Received unexpected message: op=%d\n", resp_msg->op);
        }
        message__free_unpacked(resp_msg, NULL);
    }

    close(sockfd);
    return 0;
}
