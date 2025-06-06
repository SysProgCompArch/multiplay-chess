#include <stdio.h>

#include "../client_state.h"
#include "handlers.h"

int handle_chat_broadcast(ServerMessage *msg) {
    if (msg->msg_case != SERVER_MESSAGE__MSG_CHAT_BROADCAST) {
        return -1;
    }

    if (msg->chat_broadcast && msg->chat_broadcast->message && msg->chat_broadcast->player_id) {
        add_chat_message_safe(msg->chat_broadcast->player_id, msg->chat_broadcast->message);

        // 채팅 메시지 수신 시 화면 업데이트 요청
        client_state_t *client = get_client_state();
        pthread_mutex_lock(&screen_mutex);
        client->screen_update_requested = true;
        pthread_mutex_unlock(&screen_mutex);
    }

    return 0;
}