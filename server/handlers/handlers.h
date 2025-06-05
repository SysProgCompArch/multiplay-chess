#ifndef HANDLERS_H
#define HANDLERS_H

#include "message.pb-c.h"

// 핸들러에서 사용하는 함수들 (common/network.c에서 정의)
int send_server_message(int fd, ServerMessage *msg);

// 메시지 디스패처
int dispatch_client_message(int fd, ClientMessage *msg);
int handle_unknown_message(int fd, ClientMessage *msg);

// 핸들러 함수 타입 정의
typedef int (*message_handler_t)(int fd, ClientMessage *req);

// 각 메시지 타입별 핸들러 함수들
int handle_ping_message(int fd, ClientMessage *req);
int handle_echo_message(int fd, ClientMessage *req);
int handle_match_game_message(int fd, ClientMessage *req);
int handle_chat_message(int fd, ClientMessage *req);

// 나중에 추가될 핸들러들
// int handle_move_message(int fd, ClientMessage *req);
// int handle_resign_message(int fd, ClientMessage *req);
// int handle_chat_message(int fd, ClientMessage *req);

#endif  // HANDLERS_H