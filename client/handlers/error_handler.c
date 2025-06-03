#include "handlers.h"
#include "../client_state.h"
#include "../ui.h"
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

        // 채팅 영역에도 메시지 추가
        add_chat_message_safe("System", error_msg);

        // 에러 다이얼로그 표시
        show_error_dialog("Server Error", msg->error->message);
    }

    return 0;
}