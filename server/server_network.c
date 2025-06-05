#include "server_network.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "handlers/handlers.h"
#include "handlers/match_manager.h"
#include "logger.h"
#include "network.h"

// 에러 응답을 보내는 헬퍼 함수
int send_error_response(int fd, int error_code, const char *error_message) {
    ServerMessage error_msg  = SERVER_MESSAGE__INIT;
    ErrorResponse error_resp = ERROR_RESPONSE__INIT;

    error_resp.code    = error_code;
    error_resp.message = (char *)error_message;

    error_msg.msg_case = SERVER_MESSAGE__MSG_ERROR;
    error_msg.error    = &error_resp;

    return send_server_message(fd, &error_msg);
}

// 파일 디스크립터를 논블로킹 모드로 전환
int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// 명령행 인자에서 --port 포트번호 형식을 파싱하여 포트번호를 반환 (없으면 기본값)
int parse_port_from_args(int argc, char *argv[]) {
    int port = DEFAULT_PORT;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = atoi(argv[i + 1]);
            if (port <= 0 || port > 65535) {
                LOG_FATAL("Invalid port number: %d", port);
                exit(EXIT_FAILURE);
            }
            i++;  // 포트번호 인자 스킵
        }
    }
    LOG_DEBUG("Port parsed from arguments: %d", port);
    return port;
}

// 리스닝 소켓을 생성하고, 지정한 포트에 바인드 및 리슨 상태로 만듦
int create_and_bind_listener(int port) {
    int                listener;
    struct sockaddr_in serv_addr;
    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        log_perror("socket");
        exit(EXIT_FAILURE);
    }
    LOG_DEBUG("Socket created: fd=%d", listener);

    // 주소 재사용 옵션
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    LOG_DEBUG("SO_REUSEADDR option set");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port        = htons(port);

    if (bind(listener, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        log_perror("bind");
        exit(EXIT_FAILURE);
    }
    LOG_DEBUG("Socket bound to port %d", port);

    if (listen(listener, BACKLOG) < 0) {
        log_perror("listen");
        exit(EXIT_FAILURE);
    }
    LOG_DEBUG("Socket listening with backlog %d", BACKLOG);

    // 논블로킹 모드 설정 (추천)
    set_nonblocking(listener);
    LOG_DEBUG("Listener socket set to non-blocking mode");

    return listener;
}

// epoll 인스턴스를 생성하고 리스닝 소켓을 epoll에 등록
int setup_epoll(int listener) {
    int epfd = epoll_create1(0);
    if (epfd == -1) {
        log_perror("epoll_create1");
        exit(EXIT_FAILURE);
    }
    LOG_DEBUG("Epoll instance created: fd=%d", epfd);

    // 리스닝 소켓을 epoll에 등록
    struct epoll_event ev;
    ev.events  = EPOLLIN;  // 읽기 이벤트
    ev.data.fd = listener;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listener, &ev) == -1) {
        log_perror("epoll_ctl: listener");
        exit(EXIT_FAILURE);
    }
    LOG_DEBUG("Listener socket registered to epoll");

    return epfd;
}

// 새 클라이언트 연결을 accept하고 epoll에 등록
void handle_new_connection(int listener, int epfd) {
    struct epoll_event ev;
    int                new_connections = 0;

    while (1) {
        int conn = accept(listener, NULL, NULL);
        if (conn == -1) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                break;
            } else {
                log_perror("accept");
                break;
            }
        }
        // connection sockets remain blocking for protobuf framing

        // 새 소켓을 epoll에 등록
        ev.events  = EPOLLIN;
        ev.data.fd = conn;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, conn, &ev) == -1) {
            log_perror("epoll_ctl: conn");
            close(conn);
        } else {
            LOG_INFO("New client connected: fd=%d", conn);
            new_connections++;
        }
    }

    if (new_connections > 0) {
        LOG_DEBUG("Handled %d new connections", new_connections);
    }
}

// epoll 이벤트 루프: 새 연결 및 클라이언트 이벤트를 반복적으로 처리
void event_loop(int listener, int epfd) {
    struct epoll_event events[MAX_EVENTS];
    LOG_INFO("Starting event loop...");

    while (1) {
        int nready = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nready == -1) {
            if (errno == EINTR)
                continue;
            log_perror("epoll_wait");
            break;
        }

        LOG_DEBUG("epoll_wait returned %d events", nready);

        for (int i = 0; i < nready; i++) {
            int fd = events[i].data.fd;

            // 리스닝 소켓에 이벤트 발생 → 새 클라이언트 연결
            if (fd == listener) {
                LOG_DEBUG("New connection event on listener");
                handle_new_connection(listener, epfd);
            } else if (events[i].events & EPOLLIN) {
                LOG_DEBUG("Client message event on fd=%d", fd);
                handle_client_message(fd, epfd);
            }
        }
    }

    LOG_INFO("Event loop terminated");
}

void cleanup(int listener, int epfd) {
    LOG_INFO("Cleaning up network resources");
    close(listener);
    close(epfd);
    LOG_DEBUG("Network resources cleaned up");
}

// 클라이언트 소켓에서 protobuf 메시지 수신 및 처리, 연결 종료 처리
void handle_client_message(int fd, int epfd) {
    ClientMessage *msg = receive_client_message(fd);
    if (!msg) {
        LOG_INFO("Client disconnected: fd=%d", fd);

        // 매칭 큐에서 플레이어 제거
        remove_player_from_matching(fd);

        // TODO: 진행 중인 게임이 있다면 상대방에게 알림
        ActiveGame *game = find_game_by_player_fd(fd);
        if (game) {
            LOG_INFO("Player disconnected from game %s: fd=%d", game->game_id, fd);
            // 상대방에게 연결 해제 알림 (나중에 구현)
            remove_game(game->game_id);
        }

        close(fd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
        return;
    }

    // 메시지 디스패처로 처리 위임
    int result = dispatch_client_message(fd, msg);

    // 핸들러에서 에러가 발생하면 에러 응답을 보냄
    if (result < 0) {
        LOG_WARN("Handler error for fd=%d, sending error response", fd);
        send_error_response(fd, -result, "An error occurred while processing the request");
    }

    client_message__free_unpacked(msg, NULL);
}