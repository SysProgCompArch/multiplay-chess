#include <stdio.h>

#include "../client_state.h"
#include "handlers.h"
#include "logger.h"

// 서버 메시지 라우팅
int dispatch_server_message(ServerMessage *msg) {
    if (!msg) {
        LOG_ERROR("Received null server message");
        return -1;
    }

    LOG_DEBUG("Dispatching server message type: %d", msg->msg_case);

    switch (msg->msg_case) {
        case SERVER_MESSAGE__MSG_PING_RES:
            return handle_ping_response(msg);

        case SERVER_MESSAGE__MSG_ECHO_RES:
            return handle_echo_response(msg);

        case SERVER_MESSAGE__MSG_MATCH_GAME_RES:
            return handle_match_game_response(msg);

        case SERVER_MESSAGE__MSG_CHAT_BROADCAST:
            return handle_chat_broadcast(msg);

        case SERVER_MESSAGE__MSG_OPPONENT_DISCONNECTED:
            return handle_opponent_disconnected_broadcast(msg);

        case SERVER_MESSAGE__MSG_ERROR:
            return handle_error_response(msg);

        case SERVER_MESSAGE__MSG_MOVE_RES:
            return handle_move_response(msg);

        case SERVER_MESSAGE__MSG_MOVE_BROADCAST:
            return handle_move_broadcast(msg);

        case SERVER_MESSAGE__MSG_CHECK_BROADCAST:
            return handle_check_broadcast(msg);

        default:
            LOG_WARN("Unknown server message type: %d", msg->msg_case);
            add_chat_message_safe("System", "Unknown server message");
            return 0;
    }
}