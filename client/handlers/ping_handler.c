#include "handlers.h"
#include "../client_state.h"
#include <stdio.h>

int handle_ping_response(ServerMessage *msg)
{
    if (msg->msg_case != SERVER_MESSAGE__MSG_PING_RES)
    {
        return -1;
    }

    add_chat_message_safe("Server", "Ping response received");
    return 0;
}