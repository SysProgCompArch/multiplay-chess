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

#define PORT        8080
#define MAX_EVENTS  1024
#define BACKLOG     10

// 파일 디스크립터를 논블로킹 모드로 전환
int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int listener, epfd;
    struct sockaddr_in serv_addr;

    // 1) 리스닝 소켓 생성
    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket"); exit(EXIT_FAILURE);
    }
    // 주소 재사용 옵션
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port        = htons(PORT);

    if (bind(listener, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind"); exit(EXIT_FAILURE);
    }
    if (listen(listener, BACKLOG) < 0) {
        perror("listen"); exit(EXIT_FAILURE);
    }
    // 논블로킹 모드 설정 (추천)
    set_nonblocking(listener);

    // 2) epoll 인스턴스 생성
    epfd = epoll_create1(0);
    if (epfd == -1) {
        perror("epoll_create1"); exit(EXIT_FAILURE);
    }

    // 3) 리스닝 소켓을 epoll에 등록
    struct epoll_event ev;
    ev.events = EPOLLIN;       // 읽기 이벤트
    ev.data.fd = listener;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listener, &ev) == -1) {
        perror("epoll_ctl: listener"); exit(EXIT_FAILURE);
    }

    // 이벤트 배열
    struct epoll_event events[MAX_EVENTS];

    printf("epoll 에코 서버 시작 (포트 %d)\n", PORT);

    // 4) 이벤트 루프
    while (1) {
        int nready = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nready == -1) {
            if (errno == EINTR) continue;
            perror("epoll_wait"); break;
        }

        for (int i = 0; i < nready; i++) {
            int fd = events[i].data.fd;

            // 4-1) 리스닝 소켓에 이벤트 발생 → 새 클라이언트 연결
            if (fd == listener) {
                while (1) {
                    int conn = accept(listener, NULL, NULL);
                    if (conn == -1) {
                        if (errno == EWOULDBLOCK || errno == EAGAIN) {
                            // 처리할 연결이 더 없으면 빠져나감
                            break;
                        } else {
                            perror("accept");
                            break;
                        }
                    }
                    set_nonblocking(conn);

                    // 새 소켓을 epoll에 등록 (엣지 트리거가 아니므로 EPOLLIN만)
                    ev.events = EPOLLIN;
                    ev.data.fd = conn;
                    if (epoll_ctl(epfd, EPOLL_CTL_ADD, conn, &ev) == -1) {
                        perror("epoll_ctl: conn");
                        close(conn);
                    } else {
                        printf("[fd=%d] New connection\n", conn);
                    }
                }

            // 4-2) 클라이언트 소켓에 읽기 이벤트 발생 → 데이터 처리
            } else if (events[i].events & EPOLLIN) {
                char buf[1024];
                ssize_t nbytes;
                while ((nbytes = recv(fd, buf, sizeof(buf), 0)) > 0) {
                    // heartbeat 메시지 처리
                    if (nbytes == 8 && strncmp(buf, "__ping__", 8) == 0) {
                        send(fd, "__ping__", 8, 0);
                        continue;
                    }

                    printf("[fd=%d] Received: %.*s\n", fd, (int)nbytes, buf);
                    
                    // 에코 응답 (뒤에 ' from server' 추가)
                    char sendbuf[1200];
                    int sendlen = snprintf(sendbuf, sizeof(sendbuf), "%.*s from server", (int)nbytes, buf);
                    send(fd, sendbuf, sendlen, 0);
                    printf("[fd=%d] Sent: %.*s\n", fd, sendlen, sendbuf);
                }

                if (nbytes == 0) {
                    // 클라이언트가 연결을 닫음
                    printf("[fd=%d] Client disconnected\n", fd);
                    close(fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                } else if (nbytes == -1 && errno != EWOULDBLOCK && errno != EAGAIN) {
                    // 에러 발생
                    perror("recv");
                    close(fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                }
            }
        }
    }

    // 5) 정리
    close(listener);
    close(epfd);
    return 0;
}
