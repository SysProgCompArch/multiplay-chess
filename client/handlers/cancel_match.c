#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "../client_state.h"
#include "handlers.h"
#include "logger.h"

int handle_cancel_match_response(ServerMessage *msg) {
    if (msg->msg_case != SERVER_MESSAGE__MSG_CANCEL_MATCH_RES) {
        LOG_ERROR("Invalid message type for cancel match response handler");
        return -1;
    }

    if (msg->cancel_match_res) {
        if (msg->cancel_match_res->success) {
            LOG_INFO("Matching cancelled successfully: %s",
                     msg->cancel_match_res->message ? msg->cancel_match_res->message : "no message");

            add_chat_message_safe("System", "Matching cancelled successfully");
        } else {
            LOG_WARN("Matching cancellation failed: %s",
                     msg->cancel_match_res->message ? msg->cancel_match_res->message : "no reason provided");

            add_chat_message_safe("System", msg->cancel_match_res->message ? msg->cancel_match_res->message : "Matching cancellation failed");
        }

        // 어떤 경우든 메인 화면으로 이동 (이미 cancel_matching()에서 처리됨)
        // client_state_t *client = get_client_state();
        // client->current_screen = SCREEN_MAIN;
    } else {
        LOG_ERROR("Received cancel match response but no response data");
        add_chat_message_safe("System", "Invalid server response");
    }

    return 0;
}