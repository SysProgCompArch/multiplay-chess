#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../match_manager.h"
#include "handlers.h"
#include "logger.h"
#include "network.h"

int handle_chat_message(int fd, ClientMessage *req) {
    if (req->msg_case != CLIENT_MESSAGE__MSG_CHAT) {
        LOG_ERROR("Invalid message type for chat handler: fd=%d, msg_case=%d", fd, req->msg_case);
        return -1;
    }

    LOG_INFO("Chat received from fd=%d: %s", fd, req->chat->message ? req->chat->message : "(no message)");

    ActiveGame *game = find_game_by_player_fd(fd);
    if (!game) {
        LOG_ERROR("Game not found for fd=%d", fd);
        return -1;
    }

    char *sender_id = game->white_player_fd == fd ? game->white_player_id : game->black_player_id;

    // Echo 응답 생성
    ServerMessage resp           = SERVER_MESSAGE__INIT;
    ChatBroadcast chat_broadcast = CHAT_BROADCAST__INIT;

    resp.msg_case       = SERVER_MESSAGE__MSG_CHAT_BROADCAST;
    resp.chat_broadcast = &chat_broadcast;

    // 메시지 복사
    char *chat_message       = strdup(req->chat->message);
    chat_broadcast.message   = chat_message;
    chat_broadcast.player_id = sender_id;

    // 응답 전송
    int result = send_server_message(game->white_player_fd, &resp);
    if (result < 0) {
        LOG_ERROR("Failed to send chat response to fd=%d", game->white_player_fd);
        return -1;
    }

    result = send_server_message(game->black_player_fd, &resp);
    if (result < 0) {
        LOG_ERROR("Failed to send chat response to fd=%d", game->black_player_fd);
        return -1;
    }

    // 메모리 해제
    free(chat_message);

    LOG_DEBUG("Chat response sent to fd=%d, %d", game->white_player_fd, game->black_player_fd);
    return 0;
}