#include "protocol.h"
#include "handlers/handlers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

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
        if (recvd < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        else if (recvd == 0)
        {
            return 0;
        }
        total += recvd;
    }
    return total;
}

// ServerMessage를 직렬화해서 클라이언트에게 전송
int send_server_message(int fd, ServerMessage *msg)
{
    size_t plen = server_message__get_packed_size(msg);
    uint8_t *sb = malloc(4 + plen);
    if (!sb)
    {
        perror("malloc");
        return -1;
    }

    uint32_t nl = htonl(plen);
    memcpy(sb, &nl, 4);
    server_message__pack(msg, sb + 4);

    int result = send_all(fd, sb, 4 + plen);
    free(sb);
    return result;
}

// 클라이언트로부터 메시지를 수신하고 파싱
ClientMessage *receive_client_message(int fd)
{
    // length-prefix protobuf 메시지 수신
    uint8_t lenbuf[4];
    ssize_t r = recv_all(fd, lenbuf, 4);
    if (r <= 0)
    {
        return NULL; // 연결 종료 또는 에러
    }

    uint32_t msg_len;
    memcpy(&msg_len, lenbuf, 4);
    msg_len = ntohl(msg_len);

    uint8_t *buf = malloc(msg_len);
    if (!buf)
    {
        perror("malloc");
        return NULL;
    }

    r = recv_all(fd, buf, msg_len);
    if (r <= 0)
    {
        free(buf);
        return NULL; // 연결 종료 또는 에러
    }

    // 메시지 역직렬화
    ClientMessage *req = client_message__unpack(NULL, msg_len, buf);
    free(buf);
    return req;
}

// 클라이언트 소켓에서 protobuf 메시지 수신 및 처리, 연결 종료 처리
void handle_client_message(int fd, int epfd)
{
    ClientMessage *req = receive_client_message(fd);
    if (!req)
    {
        printf("[fd=%d] Client disconnected\n", fd);
        close(fd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
        return;
    }

    // 메시지 타입에 따라 적절한 핸들러로 디스패치
    int result = 0;
    switch (req->msg_case)
    {
    case CLIENT_MESSAGE__MSG_PING:
        result = handle_ping_message(fd, req);
        break;

    case CLIENT_MESSAGE__MSG_ECHO:
        result = handle_echo_message(fd, req);
        break;

        // 나중에 추가될 메시지 타입들
        // case CLIENT_MESSAGE__MSG_MOVE:
        //     result = handle_move_message(fd, req);
        //     break;
        //
        // case CLIENT_MESSAGE__MSG_JOIN_GAME:
        //     result = handle_join_game_message(fd, req);
        //     break;

    default:
        printf("[fd=%d] Unsupported msg_case=%d\n", fd, req->msg_case);
        result = -1;
        break;
    }

    // 핸들러에서 에러가 발생하면 연결을 닫을 수도 있음
    if (result < 0)
    {
        printf("[fd=%d] Handler error, closing connection\n", fd);
        close(fd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
    }

    client_message__free_unpacked(req, NULL);
}