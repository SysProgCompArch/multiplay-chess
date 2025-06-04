#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "handlers.h"
#include "logger.h"
#include "network.h"

// ECHO 메시지 처리 핸들러
int handle_echo_message(int fd, ClientMessage *req) {
    if (req->msg_case != CLIENT_MESSAGE__MSG_ECHO) {
        LOG_ERROR("Invalid message type for echo handler: fd=%d, msg_case=%d", fd, req->msg_case);
        return -1;
    }

    LOG_INFO("Echo received from fd=%d: %s", fd, req->echo->message ? req->echo->message : "(no message)");

    // Echo 응답 생성
    ServerMessage resp      = SERVER_MESSAGE__INIT;
    EchoResponse  echo_resp = ECHO_RESPONSE__INIT;

    // 메시지 복사
    char *echo_message = malloc(strlen(req->echo->message) + 1);
    if (!echo_message) {
        LOG_ERROR("Failed to allocate memory for echo response: fd=%d", fd);
        return -1;
    }
    strcpy(echo_message, req->echo->message);
    echo_resp.message = echo_message;
    resp.msg_case     = SERVER_MESSAGE__MSG_ECHO_RES;
    resp.echo_res     = &echo_resp;

    // 응답 전송
    int result = send_server_message(fd, &resp);

    // 메모리 해제
    free(echo_message);

    if (result < 0) {
        LOG_ERROR("Failed to send echo response to fd=%d", fd);
        return -1;
    }

    LOG_DEBUG("Echo response sent to fd=%d", fd);
    return 0;
}