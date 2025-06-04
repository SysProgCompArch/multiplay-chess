#include "client_network.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
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
    LOG_INFO("Attempting to connect to server %s:%d", SERVER_HOST, SERVER_PORT);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        log_perror("socket");
        return -1;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_HOST, &serv_addr.sin_addr) <= 0) {
        log_perror("inet_pton");
        close(sockfd);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        log_perror("connect");
        close(sockfd);
        return -1;
    }

    LOG_INFO("Successfully connected to server, socket fd: %d", sockfd);
    return sockfd;
}

// 네트워크 스레드
void *network_thread(void *arg) {
    LOG_INFO("Network thread started");
    client_state_t *client = get_client_state();

    // 초기 연결 시도
    add_chat_message_safe("System", "Connecting to server...");
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
            pthread_mutex_unlock(&network_mutex);

            if (client->socket_fd >= 0) {
                close(client->socket_fd);
                client->socket_fd = -1;
            }

            add_chat_message_safe("System", "Connection lost. Reconnecting...");
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

    add_chat_message_safe("System", "Disconnected from server");
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
        add_chat_message_safe("System", "Reconnecting...");
    }

    client->socket_fd = connect_to_server();
    if (client->socket_fd < 0) {
        LOG_ERROR("Connection failed");
        pthread_mutex_lock(&network_mutex);
        client->connected    = false;
        client->reconnecting = false;
        pthread_mutex_unlock(&network_mutex);

        if (first_connect) {
            add_chat_message_safe("System", "Failed to connect to server");
        }
        return;
    }

    pthread_mutex_lock(&network_mutex);
    client->connected    = true;
    client->reconnecting = false;
    pthread_mutex_unlock(&network_mutex);

    LOG_INFO("Connection successful, sending ping");
    if (first_connect) {
        add_chat_message_safe("System", "Connected to server!");
    } else {
        add_chat_message_safe("System", "Reconnected to server!");
    }

    // 서버에 ping 메시지 전송하여 연결 확인
    ClientMessage ping_msg = CLIENT_MESSAGE__INIT;
    PingRequest   ping_req = PING_REQUEST__INIT;
    ping_req.message       = first_connect ? "Hello Server!" : "Reconnection ping";
    ping_msg.msg_case      = CLIENT_MESSAGE__MSG_PING;
    ping_msg.ping          = &ping_req;

    if (send_client_message(client->socket_fd, &ping_msg) < 0) {
        LOG_ERROR("Failed to send ping message");
        add_chat_message_safe("System", "Connection verification failed");
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
            add_chat_message_safe("System", "Failed to send match request");
            client->current_screen = SCREEN_MAIN;
        }
    } else {
        LOG_WARN("Cannot start matching: not connected to server");
        add_chat_message_safe("System", "Not connected to server");
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

    // 네트워크 스레드가 종료될 때까지 대기
    if (network_thread_id != 0) {
        pthread_join(network_thread_id, NULL);
        network_thread_id = 0;
    }

    // 소켓 정리
    client_state_t *client = get_client_state();
    if (client->socket_fd >= 0) {
        close(client->socket_fd);
        client->socket_fd = -1;
    }

    client->connected = false;
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

    chat_req.game_id   = "default_game";  // TODO: 실제 게임 ID 사용
    chat_req.player_id = client->username;
    chat_req.message   = (char *)message;

    chat_msg.msg_case = CLIENT_MESSAGE__MSG_CHAT;
    chat_msg.chat     = &chat_req;

    if (send_client_message(client->socket_fd, &chat_msg) < 0) {
        LOG_ERROR("Failed to send chat message");
        add_chat_message_safe("System", "Failed to send message");
    } else {
        LOG_DEBUG("Chat message sent successfully");
        // 로컬에도 메시지 추가 (에코용)
        add_chat_message_safe(client->username, message);
    }
}