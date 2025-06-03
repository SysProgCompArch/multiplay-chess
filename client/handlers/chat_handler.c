#include "handlers.h"
#include "../client_state.h"
#include <stdio.h>

int handle_chat_broadcast(ServerMessage *msg)
{
    if (msg->msg_case != SERVER_MESSAGE__MSG_CHAT_BROADCAST)
    {
        return -1;
    }

    if (msg->chat_broadcast && msg->chat_broadcast->message && msg->chat_broadcast->player_id)
    {
        add_chat_message_safe(msg->chat_broadcast->player_id, msg->chat_broadcast->message);
    }

    return 0;
}