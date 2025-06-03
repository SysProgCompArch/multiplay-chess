#ifndef CLIENT_CLIENT_NETWORK_H
#define CLIENT_CLIENT_NETWORK_H

#include <stdbool.h>

// 네트워크 관련 함수들
int connect_to_server();
void *network_thread(void *arg);
void cleanup_network();

// 매칭 관련
void start_matching();
void cancel_matching();

// 재연결 관련
void start_reconnect();
bool should_attempt_reconnect();

#define RECONNECT_INTERVAL 5 // 5초마다 재연결 시도

#endif // CLIENT_CLIENT_NETWORK_H