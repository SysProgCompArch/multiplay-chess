#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../match_manager.h"
#include "handlers.h"
#include "logger.h"

// 매칭 요청 처리 핸들러
int handle_match_game_message(int fd, ClientMessage *req) {
    if (!req->match_game) {
        LOG_WARN("Invalid match game request - missing data: fd=%d", fd);

        // 에러 응답 전송
        ServerMessage error_resp = SERVER_MESSAGE__INIT;
        ErrorResponse error      = ERROR_RESPONSE__INIT;
        error.code               = 1;
        error.message            = "Invalid match request data";
        error_resp.msg_case      = SERVER_MESSAGE__MSG_ERROR;
        error_resp.error         = &error;

        return send_server_message(fd, &error_resp);
    }

    MatchGameRequest *match_req = req->match_game;
    LOG_INFO("Match request from player: %s (fd=%d)",
             match_req->player_id ? match_req->player_id : "unknown", fd);

    // 플레이어 ID 검증
    if (!match_req->player_id || strlen(match_req->player_id) == 0) {
        LOG_WARN("Invalid player ID from fd=%d", fd);

        ServerMessage error_resp = SERVER_MESSAGE__INIT;
        ErrorResponse error      = ERROR_RESPONSE__INIT;
        error.code               = 2;
        error.message            = "Player ID is required";
        error_resp.msg_case      = SERVER_MESSAGE__MSG_ERROR;
        error_resp.error         = &error;

        return send_server_message(fd, &error_resp);
    }

    // 매칭 매니저에 플레이어 추가
    MatchResult result = add_player_to_matching(fd, match_req->player_id);

    // 응답 메시지 생성
    ServerMessage     response   = SERVER_MESSAGE__INIT;
    MatchGameResponse match_resp = MATCH_GAME_RESPONSE__INIT;

    switch (result.status) {
        case MATCH_STATUS_WAITING:
            // 매칭 대기 중
            LOG_INFO("Player %s added to matchmaking queue (fd=%d)", match_req->player_id, fd);

            match_resp.success       = true;
            match_resp.message       = "Waiting for opponent...";
            match_resp.game_id       = result.game_id ? result.game_id : "";
            match_resp.assigned_team = TEAM__TEAM_UNSPECIFIED;

            response.msg_case       = SERVER_MESSAGE__MSG_MATCH_GAME_RES;
            response.match_game_res = &match_resp;

            return send_server_message(fd, &response);

        case MATCH_STATUS_GAME_STARTED:
            // 게임 시작 (두 플레이어 모두에게 알림)
            LOG_INFO("Match found! Game %s started for fd=%d", result.game_id, fd);

            // 게임 정보 가져오기 (타이머 정보를 위해)
            ActiveGame *game = find_game_by_player_fd(fd);
            if (!game) {
                LOG_ERROR("Failed to find game for fd=%d", fd);
                return -1;
            }

            // 현재 플레이어에게 응답
            match_resp.success       = true;
            match_resp.message       = "Match found! Game starting...";
            match_resp.game_id       = result.game_id;
            match_resp.assigned_team = result.assigned_team;
            match_resp.opponent_name = result.opponent_name ? result.opponent_name : "";

            // 타이머 정보 추가 (밀리초를 초로 변환해서 전송)
            match_resp.time_limit_per_player = game->time_limit_per_player / 1000;
            match_resp.white_time_remaining  = game->white_time_remaining / 1000;
            match_resp.black_time_remaining  = game->black_time_remaining / 1000;

            // 게임 시작 시간 설정
            match_resp.game_start_time = malloc(sizeof(Google__Protobuf__Timestamp));
            if (match_resp.game_start_time) {
                google__protobuf__timestamp__init(match_resp.game_start_time);
                match_resp.game_start_time->seconds = game->game_start_time;
                match_resp.game_start_time->nanos   = 0;
            }

            response.msg_case       = SERVER_MESSAGE__MSG_MATCH_GAME_RES;
            response.match_game_res = &match_resp;

            int send_result = send_server_message(fd, &response);

            // 메모리 해제
            if (match_resp.game_start_time) {
                free(match_resp.game_start_time);
                match_resp.game_start_time = NULL;
            }

            // 상대방에게도 게임 시작 알림 전송
            if (send_result >= 0 && result.opponent_fd >= 0) {
                // 상대방의 상대방 이름은 현재 플레이어 이름
                char *current_player_name = NULL;
                if (game) {
                    current_player_name = (game->white_player_fd == fd) ? game->white_player_id : game->black_player_id;
                }

                MatchGameResponse opponent_resp = MATCH_GAME_RESPONSE__INIT;
                opponent_resp.success           = true;
                opponent_resp.message           = "Match found! Game starting...";
                opponent_resp.game_id           = result.game_id;
                opponent_resp.assigned_team     = (result.assigned_team == TEAM__TEAM_WHITE) ? TEAM__TEAM_BLACK : TEAM__TEAM_WHITE;
                opponent_resp.opponent_name     = current_player_name ? current_player_name : "";

                // 상대방에게도 타이머 정보 추가 (밀리초를 초로 변환해서 전송)
                opponent_resp.time_limit_per_player = game->time_limit_per_player / 1000;
                opponent_resp.white_time_remaining  = game->white_time_remaining / 1000;
                opponent_resp.black_time_remaining  = game->black_time_remaining / 1000;

                // 게임 시작 시간 설정 (상대방)
                opponent_resp.game_start_time = malloc(sizeof(Google__Protobuf__Timestamp));
                if (opponent_resp.game_start_time) {
                    google__protobuf__timestamp__init(opponent_resp.game_start_time);
                    opponent_resp.game_start_time->seconds = game->game_start_time;
                    opponent_resp.game_start_time->nanos   = 0;
                }

                ServerMessage opponent_msg  = SERVER_MESSAGE__INIT;
                opponent_msg.msg_case       = SERVER_MESSAGE__MSG_MATCH_GAME_RES;
                opponent_msg.match_game_res = &opponent_resp;

                LOG_DEBUG("Sending game start notification to opponent (fd=%d)", result.opponent_fd);
                send_server_message(result.opponent_fd, &opponent_msg);

                // 메모리 해제
                if (opponent_resp.game_start_time) {
                    free(opponent_resp.game_start_time);
                }
            }

            return send_result;

        case MATCH_STATUS_ERROR:
        default:
            // 매칭 실패
            LOG_ERROR("Matchmaking failed for player %s (fd=%d): %s",
                      match_req->player_id, fd, result.error_message ? result.error_message : "Unknown error");

            ServerMessage error_resp = SERVER_MESSAGE__INIT;
            ErrorResponse error      = ERROR_RESPONSE__INIT;
            error.code               = 3;
            error.message            = result.error_message ? result.error_message : "Matchmaking failed";
            error_resp.msg_case      = SERVER_MESSAGE__MSG_ERROR;
            error_resp.error         = &error;

            return send_server_message(fd, &error_resp);
    }
}