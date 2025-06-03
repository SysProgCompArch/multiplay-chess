#include "handlers.h"
#include "../client_state.h"
#include <stdio.h>

int handle_error_response(ServerMessage *msg)
{
    if (msg->msg_case != SERVER_MESSAGE__MSG_ERROR)
    {
        return -1;
    }

    if (msg->error && msg->error->message)
    {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Error: %s", msg->error->message);
        add_chat_message_safe("System", error_msg);
    }

    return 0;
}