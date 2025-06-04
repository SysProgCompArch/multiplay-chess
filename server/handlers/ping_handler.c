#include <stdio.h>
#include <string.h>

#include "../protocol.h"
#include "handlers.h"
#include "logger.h"
#include "network.h"

// PING 메시지 처리 핸들러
int handle_ping_message(int fd, ClientMessage *req) {
    if (req->msg_case != CLIENT_MESSAGE__MSG_PING) {
        LOG_ERROR("Invalid message type for ping handler: fd=%d, msg_case=%d", fd, req->msg_case);
        return -1;
    }

    LOG_INFO("Ping received from fd=%d: %s", fd, req->ping->message ? req->ping->message : "(no message)");

    // Ping 응답 생성
    ServerMessage resp      = SERVER_MESSAGE__INIT;
    PingResponse  ping_resp = PING_RESPONSE__INIT;
    ping_resp.message       = "Pong from server!";
    resp.msg_case           = SERVER_MESSAGE__MSG_PING_RES;
    resp.ping_res           = &ping_resp;

    // 응답 전송
    int result = send_server_message(fd, &resp);
    if (result < 0) {
        LOG_ERROR("Failed to send ping response to fd=%d", fd);
        return -1;
    }

    LOG_DEBUG("Ping response sent to fd=%d", fd);
    return 0;
}