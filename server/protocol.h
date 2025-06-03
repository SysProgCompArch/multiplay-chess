#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <sys/types.h>
#include "message.pb-c.h"

// 프로토콜 관련 함수들
ssize_t send_all(int sockfd, const void *buf, size_t len);
ssize_t recv_all(int sockfd, void *buf, size_t len);
int send_server_message(int fd, ServerMessage *msg);
ClientMessage *receive_client_message(int fd);
void handle_client_message(int fd, int epfd);

#endif // PROTOCOL_H