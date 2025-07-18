#include <stdio.h>

#include "../client_state.h"
#include "handlers.h"
#include "logger.h"

int handle_ping_response(ServerMessage *msg) {
    if (msg->msg_case != SERVER_MESSAGE__MSG_PING_RES) {
        LOG_ERROR("Invalid message type for ping response handler");
        return -1;
    }

    LOG_DEBUG("Ping response received from server: %s",
              msg->ping_res ? msg->ping_res->message : "no message");

    return 0;
}