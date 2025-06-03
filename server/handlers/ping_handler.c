#include "../protocol.h"
#include "handlers.h"
#include <stdio.h>
#include <stdlib.h>

// PING 메시지 처리 핸들러
int handle_ping_message(int fd, ClientMessage *req)
{
    if (req->msg_case != CLIENT_MESSAGE__MSG_PING)
    {
        fprintf(stderr, "[fd=%d] Invalid message type for ping handler\n", fd);
        return -1;
    }

    // PingResponse 생성
    PingResponse ping_response = PING_RESPONSE__INIT;
    ping_response.message = req->ping->message; // PingRequest의 message를 PingResponse로 복사

    ServerMessage resp = SERVER_MESSAGE__INIT;
    resp.msg_case = SERVER_MESSAGE__MSG_PING_RES;
    resp.ping_res = &ping_response;

    int result = send_server_message(fd, &resp);
    if (result < 0)
    {
        fprintf(stderr, "[fd=%d] Failed to send ping response\n", fd);
        return -1;
    }

    printf("[fd=%d] Handled PING: %s\n", fd, req->ping->message ? req->ping->message : "(null)");
    return 0;
}