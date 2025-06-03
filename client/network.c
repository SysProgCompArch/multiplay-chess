#include "client_network.h"
#include "client_state.h"
#include "handlers/handlers.h"
#include "network.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

// 서버 연결
int connect_to_server()
{
    LOG_INFO("Attempting to connect to server %s:%d", SERVER_HOST, SERVER_PORT);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        log_perror("socket");
        return -1;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_HOST, &serv_addr.sin_addr) <= 0)
    {
        log_perror("inet_pton");
        close(sockfd);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        log_perror("connect");
        close(sockfd);
        return -1;
    }

    LOG_INFO("Successfully connected to server, socket fd: %d", sockfd);
    return sockfd;
}

// 네트워크 스레드
void *network_thread(void *arg)
{
    LOG_INFO("Network thread started");
    client_state_t *client = get_client_state();

    add_chat_message_safe("System", "Connecting to server...");

    client->socket_fd = connect_to_server();
    if (client->socket_fd < 0)
    {
        LOG_ERROR("Failed to connect to server");
        add_chat_message_safe("System", "Failed to connect to server");
        pthread_mutex_lock(&network_mutex);
        client->connected = false;
        pthread_mutex_unlock(&network_mutex);
        return NULL;
    }

    pthread_mutex_lock(&network_mutex);
    client->connected = true;
    pthread_mutex_unlock(&network_mutex);

    LOG_INFO("Connection established, sending ping");
    add_chat_message_safe("System", "Connected to server!");

    // 서버에 ping 메시지 전송
    ClientMessage ping_msg = CLIENT_MESSAGE__INIT;
    PingRequest ping_req = PING_REQUEST__INIT;
    ping_req.message = "Hello Server!";
    ping_msg.msg_case = CLIENT_MESSAGE__MSG_PING; // 메시지 타입 지정
    ping_msg.ping = &ping_req;

    if (send_client_message(client->socket_fd, &ping_msg) < 0)
    {
        LOG_ERROR("Failed to send ping message");
        add_chat_message_safe("System", "Failed to send ping");
    }

    // 메시지 수신 루프
    LOG_INFO("Starting message receive loop");
    while (network_thread_running)
    {
        ServerMessage *msg = receive_server_message(client->socket_fd);
        if (!msg)
        {
            LOG_WARN("Failed to receive server message, breaking loop");
            // 연결 종료 또는 오류
            break;
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

    close(client->socket_fd);
    client->socket_fd = -1;

    add_chat_message_safe("System", "Disconnected from server");
    LOG_INFO("Network thread terminated");

    return NULL;
}

// 매칭 시작
void start_matching()
{
    LOG_INFO("Starting matchmaking");
    client_state_t *client = get_client_state();
    client->current_screen = SCREEN_MATCHING;
    client->match_start_time = time(NULL);

    // 서버에 매칭 요청 전송
    if (client->connected)
    {
        LOG_INFO("Sending match request for player: %s", client->username);
        ClientMessage match_msg = CLIENT_MESSAGE__INIT;
        MatchGameRequest match_req = MATCH_GAME_REQUEST__INIT;
        match_req.player_id = client->username;
        match_req.desired_game_id = "";                      // 새 게임
        match_msg.msg_case = CLIENT_MESSAGE__MSG_MATCH_GAME; // 메시지 타입 지정
        match_msg.match_game = &match_req;

        if (send_client_message(client->socket_fd, &match_msg) < 0)
        {
            LOG_ERROR("Failed to send match request");
            add_chat_message_safe("System", "Failed to send match request");
            client->current_screen = SCREEN_MAIN;
        }
    }
    else
    {
        LOG_WARN("Cannot start matching: not connected to server");
        add_chat_message_safe("System", "Not connected to server");
        client->current_screen = SCREEN_MAIN;
    }
}

// 매칭 취소
void cancel_matching()
{
    LOG_INFO("Cancelling matchmaking");
    client_state_t *client = get_client_state();
    client->current_screen = SCREEN_MAIN;
    // TODO: 서버에 매칭 취소 요청 전송 (필요시 구현)
}

// 네트워크 정리
void cleanup_network()
{
    LOG_INFO("Cleaning up network resources");
    if (network_thread_running)
    {
        network_thread_running = false;
        client_state_t *client = get_client_state();
        if (client->socket_fd >= 0)
        {
            close(client->socket_fd);
            client->socket_fd = -1;
        }
        LOG_INFO("Waiting for network thread to terminate");
        pthread_join(network_thread_id, NULL);
    }
    LOG_INFO("Network cleanup completed");
}