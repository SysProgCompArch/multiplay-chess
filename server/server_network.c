#include "server_network.h"
#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

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
                fprintf(stderr, "Invalid port number\n");
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

    // 리스닝 소켓을 epoll에 등록
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

            // 리스닝 소켓에 이벤트 발생 → 새 클라이언트 연결
            if (fd == listener)
            {
                handle_new_connection(listener, epfd);
            }
            else if (events[i].events & EPOLLIN)
            {
                handle_client_message(fd, epfd);
            }
        }
    }
}

void cleanup(int listener, int epfd)
{
    close(listener);
    close(epfd);
}