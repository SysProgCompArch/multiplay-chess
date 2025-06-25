#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../match_manager.h"
#include "handlers.h"
#include "logger.h"

// 매칭 취소 요청 처리 핸들러
int handle_cancel_match_message(int fd, ClientMessage *req) {
    if (!req->cancel_match) {
        LOG_WARN("Invalid cancel match request - missing data: fd=%d", fd);

        // 에러 응답 전송
        ServerMessage error_resp = SERVER_MESSAGE__INIT;
        ErrorResponse error      = ERROR_RESPONSE__INIT;
        error.code               = 1;
        error.message            = "Invalid cancel match request data";
        error_resp.msg_case      = SERVER_MESSAGE__MSG_ERROR;
        error_resp.error         = &error;

        return send_server_message(fd, &error_resp);
    }

    CancelMatchRequest *cancel_req = req->cancel_match;
    LOG_INFO("Cancel match request from player: %s (fd=%d)",
             cancel_req->player_id ? cancel_req->player_id : "unknown", fd);

    // 플레이어 ID 검증
    if (!cancel_req->player_id || strlen(cancel_req->player_id) == 0) {
        LOG_WARN("Invalid player ID in cancel match request from fd=%d", fd);

        ServerMessage error_resp = SERVER_MESSAGE__INIT;
        ErrorResponse error      = ERROR_RESPONSE__INIT;
        error.code               = 2;
        error.message            = "Player ID is required";
        error_resp.msg_case      = SERVER_MESSAGE__MSG_ERROR;
        error_resp.error         = &error;

        return send_server_message(fd, &error_resp);
    }

    // 매칭 매니저에서 플레이어 제거
    int removal_result = remove_player_from_matching(fd);

    // 응답 메시지 생성
    ServerMessage       response    = SERVER_MESSAGE__INIT;
    CancelMatchResponse cancel_resp = CANCEL_MATCH_RESPONSE__INIT;

    if (removal_result == 0) {
        // 매칭 취소 성공
        LOG_INFO("Player %s successfully removed from matching queue (fd=%d)", cancel_req->player_id, fd);

        cancel_resp.player_id = cancel_req->player_id;
        cancel_resp.success   = true;
        cancel_resp.message   = "Matching cancelled successfully";

        response.msg_case         = SERVER_MESSAGE__MSG_CANCEL_MATCH_RES;
        response.cancel_match_res = &cancel_resp;

        return send_server_message(fd, &response);
    } else {
        // 매칭 취소 실패 (플레이어가 대기 큐에 없음)
        LOG_WARN("Player %s not found in matching queue (fd=%d)", cancel_req->player_id, fd);

        cancel_resp.player_id = cancel_req->player_id;
        cancel_resp.success   = false;
        cancel_resp.message   = "Player not in matching queue";

        response.msg_case         = SERVER_MESSAGE__MSG_CANCEL_MATCH_RES;
        response.cancel_match_res = &cancel_resp;

        return send_server_message(fd, &response);
    }
}