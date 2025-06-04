#include <stdio.h>

#include "handlers.h"
#include "logger.h"

// 클라이언트 메시지 라우팅
int dispatch_client_message(int fd, ClientMessage *msg) {
    if (!msg) {
        LOG_ERROR("Received null client message");
        return -1;
    }

    LOG_DEBUG("Dispatching client message type: %d from fd=%d", msg->msg_case, fd);

    switch (msg->msg_case) {
        case CLIENT_MESSAGE__MSG_PING:
            LOG_DEBUG("Handling PING message from fd=%d", fd);
            return handle_ping_message(fd, msg);

        case CLIENT_MESSAGE__MSG_ECHO:
            LOG_DEBUG("Handling ECHO message from fd=%d", fd);
            return handle_echo_message(fd, msg);

        case CLIENT_MESSAGE__MSG_MATCH_GAME:
            LOG_DEBUG("Handling MATCH_GAME message from fd=%d", fd);
            return handle_match_game_message(fd, msg);

            // 나중에 추가될 메시지 타입들
            // case CLIENT_MESSAGE__MSG_MOVE:
            //     return handle_move_message(fd, msg);
            //
            // case CLIENT_MESSAGE__MSG_JOIN_GAME:
            //     return handle_join_game_message(fd, msg);

        default:
            LOG_WARN("Unsupported message type from fd=%d: msg_case=%d", fd, msg->msg_case);
            return handle_unknown_message(fd, msg);
    }
}

// 알 수 없는 메시지 타입 처리
int handle_unknown_message(int fd, ClientMessage *msg) {
    LOG_WARN("Unknown message type: %d", msg->msg_case);

    // 에러 응답 생성 및 전송
    ServerMessage error_resp = SERVER_MESSAGE__INIT;
    ErrorResponse error      = ERROR_RESPONSE__INIT;
    error.message            = "Unsupported message type";
    error_resp.msg_case      = SERVER_MESSAGE__MSG_ERROR;
    error_resp.error         = &error;

    int result = send_server_message(fd, &error_resp);
    if (result < 0) {
        LOG_ERROR("Failed to send error response to fd=%d", fd);
        return -1;
    } else {
        LOG_DEBUG("Error response sent successfully to fd=%d", fd);
        return 0;  // 에러 응답을 성공적으로 보냈으므로 연결 유지
    }
}