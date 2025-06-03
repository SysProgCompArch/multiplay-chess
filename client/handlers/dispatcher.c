#include "handlers.h"
#include "../client_state.h"
#include <stdio.h>

// 서버 메시지 라우팅
int dispatch_server_message(ServerMessage *msg)
{
    if (!msg)
        return -1;

    switch (msg->msg_case)
    {
    case SERVER_MESSAGE__MSG_PING_RES:
        return handle_ping_response(msg);

    case SERVER_MESSAGE__MSG_ECHO_RES:
        return handle_echo_response(msg);

    case SERVER_MESSAGE__MSG_MATCH_GAME_RES:
        return handle_match_game_response(msg);

    case SERVER_MESSAGE__MSG_CHAT_BROADCAST:
        return handle_chat_broadcast(msg);

    case SERVER_MESSAGE__MSG_ERROR:
        return handle_error_response(msg);

    default:
        add_chat_message_safe("System", "Unknown server message");
        return 0;
    }
}