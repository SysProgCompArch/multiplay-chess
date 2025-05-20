#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/epoll.h>
#include <fcntl.h>
#include <stdint.h>
#include <protobuf-c/protobuf-c.h>
#include "message.pb-c.h"

#define DEFAULT_PORT 8080
#define MAX_EVENTS 1024
#define BACKLOG 10

// 파일 디스크립터를 논블로킹 모드로 전환
int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// 명령행 인자에서 --port 포트번호 형식을 파싱하여 포트번호를 반환 (없으면 기본값)
int parse_port_from_args(int argc, char *argv[])
{
    int port = DEFAULT_PORT;
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc)
        {
            port = atoi(argv[i + 1]);
            if (port <= 0 || port > 65535)
            {
                fprintf(stderr, "잘못된 포트 번호입니다.\n");
                exit(EXIT_FAILURE);
            }
            i++; // 포트번호 인자 스킵
        }
    }
    return port;
}

// 리스닝 소켓을 생성하고, 지정한 포트에 바인드 및 리슨 상태로 만듦
int create_and_bind_listener(int port)
{
    int listener;
    struct sockaddr_in serv_addr;
    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    // 주소 재사용 옵션
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(listener, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if (listen(listener, BACKLOG) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    // 논블로킹 모드 설정 (추천)
    set_nonblocking(listener);

    return listener;
}

// epoll 인스턴스를 생성하고 리스닝 소켓을 epoll에 등록
int setup_epoll(int listener)
{
    int epfd = epoll_create1(0);
    if (epfd == -1)
    {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    // 3) 리스닝 소켓을 epoll에 등록
    struct epoll_event ev;
    ev.events = EPOLLIN; // 읽기 이벤트
    ev.data.fd = listener;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listener, &ev) == -1)
    {
        perror("epoll_ctl: listener");
        exit(EXIT_FAILURE);
    }

    return epfd;
}

// 모든 데이터를 전송할 때까지 반복하여 전송한다
static ssize_t send_all(int sockfd, const void *buf, size_t len)
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
static ssize_t recv_all(int sockfd, void *buf, size_t len)
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

// 새 클라이언트 연결을 accept하고 epoll에 등록
void handle_new_connection(int listener, int epfd)
{
    struct epoll_event ev;
    while (1)
    {
        int conn = accept(listener, NULL, NULL);
        if (conn == -1)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                break;
            }
            else
            {
                perror("accept");
                break;
            }
        }
        // connection sockets remain blocking for protobuf framing

        // 새 소켓을 epoll에 등록
        ev.events = EPOLLIN;
        ev.data.fd = conn;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, conn, &ev) == -1)
        {
            perror("epoll_ctl: conn");
            close(conn);
        }
        else
        {
            printf("[fd=%d] New connection\n", conn);
        }
    }
}

// 클라이언트 소켓에서 protobuf 메시지 수신 및 처리, 에코 응답, 연결 종료 처리
void handle_client_event(int fd, int epfd)
{
    // length-prefix protobuf 메시지 수신
    uint8_t lenbuf[4];
    ssize_t r = recv_all(fd, lenbuf, 4);
    if (r <= 0)
    {
        printf("[fd=%d] Client disconnected\n", fd);
        close(fd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
        return;
    }
    uint32_t msg_len;
    memcpy(&msg_len, lenbuf, 4);
    msg_len = ntohl(msg_len);
    uint8_t *buf = malloc(msg_len);
    if (!buf)
    {
        perror("malloc");
        return;
    }
    r = recv_all(fd, buf, msg_len);
    if (r <= 0)
    {
        free(buf);
        printf("[fd=%d] Client disconnected during payload\n", fd);
        close(fd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
        return;
    }
    // 메시지 역직렬화
    Message *req = message__unpack(NULL, msg_len, buf);
    free(buf);
    if (!req)
    {
        fprintf(stderr, "[fd=%d] Failed to unpack message\n", fd);
        return;
    }
    // PING 처리
    if (req->op == OP_CODE__PING)
    {
        Message resp = MESSAGE__INIT;
        resp.op = OP_CODE__PING;
        size_t plen = message__get_packed_size(&resp);
        uint8_t *sb = malloc(4 + plen);
        uint32_t nl = htonl(plen);
        memcpy(sb, &nl, 4);
        message__pack(&resp, sb + 4);
        send_all(fd, sb, 4 + plen);
        free(sb);
    }
    // Echo 처리
    else if (req->op == OP_CODE__ECHO_MSG && req->data_case == MESSAGE__DATA_ECHO)
    {
        printf("[fd=%d] ECHO_MSG\n", fd);
        printf("[fd=%d] Received: %s\n", fd, req->echo->msg);
        EchoData echo_data = ECHO_DATA__INIT;
        echo_data.msg = req->echo->msg;
        Message resp = MESSAGE__INIT;
        resp.op = OP_CODE__ECHO_MSG;
        resp.data_case = MESSAGE__DATA_ECHO;
        resp.echo = &echo_data;
        size_t plen = message__get_packed_size(&resp);
        uint8_t *sb = malloc(4 + plen);
        uint32_t nl = htonl(plen);
        memcpy(sb, &nl, 4);
        message__pack(&resp, sb + 4);
        send_all(fd, sb, 4 + plen);
        printf("[fd=%d] Sent: %s\n", fd, echo_data.msg);
        free(sb);
    }
    else
    {
        printf("[fd=%d] Unsupported op=%d\n", fd, req->op);
    }
    message__free_unpacked(req, NULL);
}

// epoll 이벤트 루프: 새 연결 및 클라이언트 이벤트를 반복적으로 처리
void event_loop(int listener, int epfd)
{
    struct epoll_event events[MAX_EVENTS];
    while (1)
    {
        int nready = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nready == -1)
        {
            if (errno == EINTR)
                continue;
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < nready; i++)
        {
            int fd = events[i].data.fd;

            // 4-1) 리스닝 소켓에 이벤트 발생 → 새 클라이언트 연결
            if (fd == listener)
            {
                handle_new_connection(listener, epfd);
            }
            else if (events[i].events & EPOLLIN)
            {
                handle_client_event(fd, epfd);
            }
        }
    }
}

void cleanup(int listener, int epfd)
{
    close(listener);
    close(epfd);
}

int main(int argc, char *argv[])
{
    int port = parse_port_from_args(argc, argv);
    int listener = create_and_bind_listener(port);
    int epfd = setup_epoll(listener);
    printf("epoll 에코 서버 시작 (포트 %d)\n", port);
    event_loop(listener, epfd);
    cleanup(listener, epfd);
    return 0;
}
