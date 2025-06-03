#ifndef CLIENT_CLIENT_NETWORK_H
#define CLIENT_CLIENT_NETWORK_H

// 네트워크 관련 함수들
int connect_to_server();
void *network_thread(void *arg);
void cleanup_network();

// 매칭 관련
void start_matching();
void cancel_matching();

#endif // CLIENT_CLIENT_NETWORK_H