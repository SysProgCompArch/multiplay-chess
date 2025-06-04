#ifndef COMMON_NETWORK_H
#define COMMON_NETWORK_H

#include <stdint.h>
#include <sys/types.h>

#include "message.pb-c.h"

// 공통 네트워크 함수들
ssize_t send_all(int sockfd, const void *buf, size_t len);
ssize_t recv_all(int sockfd, void *buf, size_t len);

// 메시지 송수신 함수들
int            send_client_message(int fd, ClientMessage *msg);
int            send_server_message(int fd, ServerMessage *msg);
ClientMessage *receive_client_message(int fd);
ServerMessage *receive_server_message(int fd);

#endif  // COMMON_NETWORK_H