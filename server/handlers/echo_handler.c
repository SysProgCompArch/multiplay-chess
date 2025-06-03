#include "../protocol.h"
#include "handlers.h"
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

    printf("[fd=%d] Received: %s\n", fd, req->echo->message);

    // 보낼 메시지에 ' from server' 추가
    const char *orig = req->echo->message;
    const char *suffix = " from server";
    size_t orig_len = strlen(orig);
    size_t suffix_len = strlen(suffix);
    size_t new_len = orig_len + suffix_len;

    char *send_str = malloc(new_len + 1);
    if (!send_str)
    {
        perror("malloc");
        return -1;
    }

    memcpy(send_str, orig, orig_len);
    memcpy(send_str + orig_len, suffix, suffix_len);
    send_str[new_len] = '\0';

    EchoResponse echo_data = ECHO_RESPONSE__INIT;
    echo_data.message = send_str;

    ServerMessage resp = SERVER_MESSAGE__INIT;
    resp.msg_case = SERVER_MESSAGE__MSG_ECHO_RES;
    resp.echo_res = &echo_data;

    int result = send_server_message(fd, &resp);
    if (result < 0)
    {
        fprintf(stderr, "[fd=%d] Failed to send echo response\n", fd);
        free(send_str);
        return -1;
    }

    printf("[fd=%d] Sent: %s\n", fd, send_str);
    free(send_str);
    return 0;
}