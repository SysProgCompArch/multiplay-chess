#ifndef CLIENT_HANDLERS_H
#define CLIENT_HANDLERS_H

#include "message.pb-c.h"

// 클라이언트 핸들러 함수 타입 정의
typedef int (*server_message_handler_t)(ServerMessage *msg);

// 각 메시지 타입별 핸들러 함수들
int handle_ping_response(ServerMessage *msg);
int handle_echo_response(ServerMessage *msg);
int handle_match_game_response(ServerMessage *msg);
int handle_chat_broadcast(ServerMessage *msg);
int handle_opponent_disconnected_broadcast(ServerMessage *msg);
int handle_error_response(ServerMessage *msg);

// 메인 핸들러 디스패처
int dispatch_server_message(ServerMessage *msg);

#endif  // CLIENT_HANDLERS_H