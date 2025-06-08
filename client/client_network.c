#include "client_network.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "client_state.h"
#include "handlers/handlers.h"
#include "logger.h"
#include "network.h"

// 서버 연결
int connect_to_server() {
    client_state_t *client = get_client_state();

    LOG_INFO("Attempting to connect to server %s:%d", client->server_host, client->server_port);

    // getaddrinfo를 사용하여 DNS 해결 및 주소 변환
    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;      // IPv4만 사용
    hints.ai_socktype = SOCK_STREAM;  // TCP 소켓

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", client->server_port);

    int gai_status = getaddrinfo(client->server_host, port_str, &hints, &result);
    if (gai_status != 0) {
        LOG_ERROR("getaddrinfo failed: %s", gai_strerror(gai_status));
        return -1;
    }

    int sockfd = -1;
    // 반환된 주소들을 순회하며 연결 시도
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd < 0) {
            continue;  // 다음 주소로 시도
        }

        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) {
            LOG_INFO("Successfully connected to server, socket fd: %d", sockfd);
            break;  // 연결 성공
        }

        // 연결 실패 시 소켓 닫고 다음 주소로 시도
        log_perror("connect");
        close(sockfd);
        sockfd = -1;
    }

    freeaddrinfo(result);

    if (sockfd < 0) {
        LOG_ERROR("Could not connect to server %s:%d", client->server_host, client->server_port);
        return -1;
    }

    return sockfd;
}

// 네트워크 스레드
void *network_thread(void *arg) {
    LOG_INFO("Network thread started");

    // 스레드 취소 활성화
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    // 시그널 마스크 설정 - 메인 스레드에서만 시그널을 처리하도록 함
    sigset_t set;
    sigfillset(&set);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    client_state_t *client = get_client_state();

    // 초기 연결 시도
    LOG_INFO("Connecting to server...");
    client->last_reconnect_attempt = 0;  // 초기 연결은 즉시 시도

    while (network_thread_running) {
        // 연결되지 않은 상태이고 재연결이 필요한 경우
        if (!client->connected) {
            if (should_attempt_reconnect()) {
                start_reconnect();
            } else {
                // 1초 대기 후 다시 확인
                usleep(1000000);  // 1초
                continue;
            }
        }

        if (!client->connected) {
            // 재연결 실패 시 1초 대기
            usleep(1000000);
            continue;
        }

        // 메시지 수신 루프
        ServerMessage *msg = receive_server_message(client->socket_fd);
        if (!msg) {
            LOG_WARN("Failed to receive server message, connection lost");
            // 연결 종료 또는 오류
            pthread_mutex_lock(&network_mutex);
            client->connected = false;

            // 게임 중이었다면 연결 끊김 상태로 설정
            if (client->current_screen == SCREEN_GAME) {
                client->connection_lost = true;
                strcpy(client->disconnect_message, "Connection lost");

                // 화면 업데이트 요청
                client->screen_update_requested = true;
            }
            pthread_mutex_unlock(&network_mutex);

            if (client->socket_fd >= 0) {
                close(client->socket_fd);
                client->socket_fd = -1;
            }

            // 종료 요청이 있었다면 재연결 메시지 출력하지 않고 루프 탈출
            if (!network_thread_running) {
                LOG_DEBUG("Network thread shutdown requested during connection loss");
                break;
            }

            LOG_INFO("Connection lost. Reconnecting...");
            continue;
        }

        LOG_DEBUG("Received server message, dispatching to handler");
        // 핸들러로 메시지 처리
        dispatch_server_message(msg);
        server_message__free_unpacked(msg, NULL);
    }

    LOG_INFO("Network thread ending, cleaning up connection");
    pthread_mutex_lock(&network_mutex);
    client->connected = false;
    pthread_mutex_unlock(&network_mutex);

    if (client->socket_fd >= 0) {
        close(client->socket_fd);
        client->socket_fd = -1;
    }

    LOG_INFO("Network thread terminated");

    return NULL;
}

// 재연결 시작
void start_reconnect() {
    LOG_INFO("Attempting to reconnect to server");
    client_state_t *client = get_client_state();

    pthread_mutex_lock(&network_mutex);
    client->reconnecting           = true;
    client->last_reconnect_attempt = time(NULL);
    pthread_mutex_unlock(&network_mutex);

    // 처음 연결 시도인지 확인
    static bool first_connect = true;
    if (first_connect) {
        first_connect = false;
        // 초기 연결 시도 메시지는 이미 출력됨
    } else {
        LOG_INFO("Reconnecting...");
    }

    client->socket_fd = connect_to_server();
    if (client->socket_fd < 0) {
        LOG_ERROR("Connection failed");
        pthread_mutex_lock(&network_mutex);
        client->connected    = false;
        client->reconnecting = false;
        pthread_mutex_unlock(&network_mutex);

        if (first_connect) {
            LOG_INFO("Failed to connect to server");
        }
        return;
    }

    pthread_mutex_lock(&network_mutex);
    client->connected    = true;
    client->reconnecting = false;
    pthread_mutex_unlock(&network_mutex);

    LOG_INFO("Connection successful, sending ping");
    if (first_connect) {
        LOG_INFO("Connected to server!");
    } else {
        LOG_INFO("Reconnected to server!");
    }

    // 서버에 ping 메시지 전송하여 연결 확인
    ClientMessage ping_msg = CLIENT_MESSAGE__INIT;
    PingRequest   ping_req = PING_REQUEST__INIT;
    ping_req.message       = first_connect ? "Hello Server!" : "Reconnection ping";
    ping_msg.msg_case      = CLIENT_MESSAGE__MSG_PING;
    ping_msg.ping          = &ping_req;

    if (send_client_message(client->socket_fd, &ping_msg) < 0) {
        LOG_ERROR("Failed to send ping message");
        pthread_mutex_lock(&network_mutex);
        client->connected = false;
        pthread_mutex_unlock(&network_mutex);
        close(client->socket_fd);
        client->socket_fd = -1;
    }
}

// 재연결을 시도해야 하는지 확인
bool should_attempt_reconnect() {
    client_state_t *client       = get_client_state();
    time_t          current_time = time(NULL);

    // 이미 재연결 중이면 시도하지 않음
    if (client->reconnecting) {
        return false;
    }

    // 마지막 재연결 시도 시간으로부터 RECONNECT_INTERVAL초가 지났는지 확인
    if (current_time - client->last_reconnect_attempt >= RECONNECT_INTERVAL) {
        return true;
    }

    return false;
}

// 매칭 시작
void start_matching() {
    LOG_INFO("Starting matchmaking");
    client_state_t *client   = get_client_state();
    client->current_screen   = SCREEN_MATCHING;
    client->match_start_time = time(NULL);

    // 서버에 매칭 요청 전송
    if (client->connected) {
        LOG_INFO("Sending match request for player: %s", client->username);
        ClientMessage    match_msg = CLIENT_MESSAGE__INIT;
        MatchGameRequest match_req = MATCH_GAME_REQUEST__INIT;
        match_req.player_id        = client->username;
        match_req.desired_game_id  = "";                              // 새 게임
        match_msg.msg_case         = CLIENT_MESSAGE__MSG_MATCH_GAME;  // 메시지 타입 지정
        match_msg.match_game       = &match_req;

        if (send_client_message(client->socket_fd, &match_msg) < 0) {
            LOG_ERROR("Failed to send match request");
            client->current_screen = SCREEN_MAIN;
        }
    } else {
        LOG_WARN("Cannot start matching: not connected to server");
        client->current_screen = SCREEN_MAIN;
    }
}

// 매칭 취소
void cancel_matching() {
    LOG_INFO("Cancelling matchmaking");
    client_state_t *client = get_client_state();
    client->current_screen = SCREEN_MAIN;
    // TODO: 서버에 매칭 취소 요청 전송 (필요시 구현)
}

// 네트워크 정리
void cleanup_network() {
    LOG_INFO("Cleaning up network resources");
    network_thread_running = false;

    // 소켓을 먼저 닫아서 네트워크 스레드의 블로킹을 해제
    client_state_t *client = get_client_state();
    if (client->socket_fd >= 0) {
        LOG_DEBUG("Closing socket to interrupt network thread");
        close(client->socket_fd);
        client->socket_fd = -1;
    }

    pthread_mutex_lock(&network_mutex);
    client->connected = false;
    pthread_mutex_unlock(&network_mutex);

    // 네트워크 스레드가 종료될 때까지 대기 (타임아웃 포함)
    if (network_thread_id != 0) {
        LOG_DEBUG("Waiting for network thread to terminate");

        // 잠시 대기하여 네트워크 스레드가 자연스럽게 종료되도록 함
        usleep(100000);  // 100ms 대기

        // 네트워크 스레드가 여전히 블로킹되어 있을 수 있으므로 강제 종료
        LOG_WARN("Attempting to cancel network thread to ensure clean shutdown");
        pthread_cancel(network_thread_id);
        pthread_join(network_thread_id, NULL);
        LOG_DEBUG("Network thread cancelled and joined successfully");

        network_thread_id = 0;
    }

    LOG_INFO("Network cleanup completed");
}

// 채팅 메시지 전송
void send_chat_message(const char *message) {
    LOG_INFO("Sending chat message: %s", message);
    client_state_t *client = get_client_state();

    if (!client->connected) {
        LOG_WARN("Cannot send chat message: not connected to server");
        add_chat_message_safe("System", "Cannot send message: not connected");
        return;
    }

    if (!message || strlen(message) == 0) {
        LOG_WARN("Cannot send empty chat message");
        return;
    }

    // 채팅 메시지 생성
    ClientMessage chat_msg = CLIENT_MESSAGE__INIT;
    ChatRequest   chat_req = CHAT_REQUEST__INIT;

    chat_req.message = (char *)message;

    chat_msg.msg_case = CLIENT_MESSAGE__MSG_CHAT;
    chat_msg.chat     = &chat_req;

    if (send_client_message(client->socket_fd, &chat_msg) < 0) {
        LOG_ERROR("Failed to send chat message");
    } else {
        LOG_DEBUG("Chat message sent successfully");
    }
}

// 체스 좌표를 배열 인덱스로 변환 (예: "a1" -> (0, 0))
static bool parse_chess_coordinate_network(const char *coord, int *x, int *y) {
    if (!coord || strlen(coord) != 2) {
        return false;
    }

    char file = coord[0];  // a-h
    char rank = coord[1];  // 1-8

    if (file < 'a' || file > 'h' || rank < '1' || rank > '8') {
        return false;
    }

    *x = file - 'a';  // 0-7
    *y = rank - '1';  // 0-7

    return true;
}

// 기물 이동 요청 전송
int send_move_request(const char *from, const char *to) {
    LOG_INFO("Sending move request: %s -> %s", from, to);
    client_state_t *client = get_client_state();

    if (!client->connected) {
        LOG_WARN("Cannot send move request: not connected to server");
        add_chat_message_safe("System", "Cannot send move: not connected");
        return -1;
    }

    if (!from || !to) {
        LOG_ERROR("Invalid move coordinates: from=%s, to=%s", from, to);
        return -1;
    }

    // 좌표 유효성 검사
    int from_x, from_y, to_x, to_y;
    if (!parse_chess_coordinate_network(from, &from_x, &from_y)) {
        LOG_ERROR("Invalid 'from' coordinate: %s", from);
        add_chat_message_safe("System", "Invalid source coordinate");
        return -1;
    }

    if (!parse_chess_coordinate_network(to, &to_x, &to_y)) {
        LOG_ERROR("Invalid 'to' coordinate: %s", to);
        add_chat_message_safe("System", "Invalid destination coordinate");
        return -1;
    }

    // MoveRequest 메시지 생성
    ClientMessage client_msg = CLIENT_MESSAGE__INIT;
    MoveRequest   move_req   = MOVE_REQUEST__INIT;

    move_req.from = (char *)from;
    move_req.to   = (char *)to;

    // TODO: timestamp 설정 (필요시)

    client_msg.msg_case = CLIENT_MESSAGE__MSG_MOVE;
    client_msg.move     = &move_req;

    // 메시지 전송
    if (send_client_message(client->socket_fd, &client_msg) < 0) {
        LOG_ERROR("Failed to send move request");
        add_chat_message_safe("System", "Failed to send move request");
        return -1;
    }

    LOG_DEBUG("Move request sent successfully: %s -> %s", from, to);
    return 0;
}