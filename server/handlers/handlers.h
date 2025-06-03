#ifndef HANDLERS_H
#define HANDLERS_H

#include "message.pb-c.h"

// 핸들러 함수 타입 정의
typedef int (*message_handler_t)(int fd, ClientMessage *req);

// 각 메시지 타입별 핸들러 함수들
int handle_ping_message(int fd, ClientMessage *req);
int handle_echo_message(int fd, ClientMessage *req);

// 나중에 추가될 핸들러들
// int handle_move_message(int fd, ClientMessage *req);
// int handle_join_game_message(int fd, ClientMessage *req);
// int handle_chat_message(int fd, ClientMessage *req);

#endif // HANDLERS_H