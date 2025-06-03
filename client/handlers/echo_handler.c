#include "handlers.h"
#include "../client_state.h"
#include <stdio.h>
#include <string.h>

int handle_echo_response(ServerMessage *msg)
{
    if (msg->msg_case != SERVER_MESSAGE__MSG_ECHO_RES)
    {
        return -1;
    }

    if (msg->echo_res && msg->echo_res->message)
    {
        char echo_msg[256];
        snprintf(echo_msg, sizeof(echo_msg), "Echo: %s", msg->echo_res->message);
        add_chat_message_safe("Server", echo_msg);
    }

    return 0;
}