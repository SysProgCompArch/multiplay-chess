#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "handlers.h"
#include "logger.h"
#include "match_manager.h"
#include "network.h"

// ECHO 메시지 처리 핸들러
int handle_chat_message(int fd, ClientMessage *req) {
    if (req->msg_case != CLIENT_MESSAGE__MSG_CHAT) {
        LOG_ERROR("Invalid message type for chat handler: fd=%d, msg_case=%d", fd, req->msg_case);
        return -1;
    }

    char *player_id = find_player_id_by_fd(fd);
    if (!player_id) {
        LOG_ERROR("Player ID not found for fd=%d", fd);
        return -1;
    }

    LOG_INFO("Chat received from fd=%d: %s", fd, req->chat->message ? req->chat->message : "(no message)");

    // Echo 응답 생성
    ServerMessage resp           = SERVER_MESSAGE__INIT;
    ChatBroadcast chat_broadcast = CHAT_BROADCAST__INIT;

    // 메시지 복사
    char *chat_message     = strdup(req->chat->message);
    chat_broadcast.message = chat_message;
    resp.msg_case          = SERVER_MESSAGE__MSG_CHAT_BROADCAST;
    resp.chat_broadcast    = &chat_broadcast;

    // 응답 전송
    int result = send_server_message(fd, &resp);

    // 메모리 해제
    free(chat_message);

    if (result < 0) {
        LOG_ERROR("Failed to send chat response to fd=%d", fd);
        return -1;
    }

    LOG_DEBUG("Chat response sent to fd=%d", fd);
    return 0;
}