#include "../protocol.h"
#include "handlers.h"
#include "network.h"
#include <stdio.h>
#include <string.h>

// PING 메시지 처리 핸들러
int handle_ping_message(int fd, ClientMessage *req)
{
    if (req->msg_case != CLIENT_MESSAGE__MSG_PING)
    {
        fprintf(stderr, "[fd=%d] Invalid message type for ping handler\n", fd);
        return -1;
    }

    printf("[fd=%d] Ping: %s\n", fd, req->ping->message);

    // Ping 응답 생성
    ServerMessage resp = SERVER_MESSAGE__INIT;
    PingResponse ping_resp = PING_RESPONSE__INIT;
    ping_resp.message = "Pong from server!";
    resp.msg_case = SERVER_MESSAGE__MSG_PING_RES;
    resp.ping_res = &ping_resp;

    // 응답 전송
    int result = send_server_message(fd, &resp);
    if (result < 0)
    {
        printf("[fd=%d] Failed to send ping response\n", fd);
        return -1;
    }

    printf("[fd=%d] Ping response sent\n", fd);
    return 0;
}