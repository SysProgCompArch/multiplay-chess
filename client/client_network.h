#ifndef CLIENT_CLIENT_NETWORK_H
#define CLIENT_CLIENT_NETWORK_H

#include <stdbool.h>

// 서버 연결 설정
#define SERVER_DEFAULT_HOST "127.0.0.1"
#define SERVER_DEFAULT_PORT 8080

// 네트워크 관련 함수들
int   connect_to_server();
void *network_thread(void *arg);
void  cleanup_network();

// 매칭 관련
void start_matching();
void cancel_matching();

// 재연결 관련
void start_reconnect();
bool should_attempt_reconnect();

// 채팅 관련
void send_chat_message(const char *message);

// 게임 관련
int send_move_request(const char *from, const char *to);
int send_resign_request();

#define RECONNECT_INTERVAL 5  // 5초마다 재연결 시도

#endif  // CLIENT_CLIENT_NETWORK_H