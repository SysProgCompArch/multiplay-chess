#include "../protocol.h"
#include "handlers.h"
#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ECHO 메시지 처리 핸들러
int handle_echo_message(int fd, ClientMessage *req)
{
    if (req->msg_case != CLIENT_MESSAGE__MSG_ECHO)
    {
        fprintf(stderr, "[fd=%d] Invalid message type for echo handler\n", fd);
        return -1;
    }

    printf("[fd=%d] Echo: %s\n", fd, req->echo->message);

    // Echo 응답 생성
    ServerMessage resp = SERVER_MESSAGE__INIT;
    EchoResponse echo_resp = ECHO_RESPONSE__INIT;

    // 메시지 복사
    char *echo_message = malloc(strlen(req->echo->message) + 1);
    if (!echo_message)
    {
        printf("[fd=%d] Failed to allocate memory for echo response\n", fd);
        return -1;
    }
    strcpy(echo_message, req->echo->message);
    echo_resp.message = echo_message;
    resp.msg_case = SERVER_MESSAGE__MSG_ECHO_RES;
    resp.echo_res = &echo_resp;

    // 응답 전송
    int result = send_server_message(fd, &resp);

    // 메모리 해제
    free(echo_message);

    if (result < 0)
    {
        printf("[fd=%d] Failed to send echo response\n", fd);
        return -1;
    }

    printf("[fd=%d] Echo response sent\n", fd);
    return 0;
}