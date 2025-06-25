#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../client_state.h"
#include "../config.h"
#include "../game_state.h"
#include "handlers.h"
#include "logger.h"

int handle_match_game_response(ServerMessage *msg) {
    if (msg->msg_case != SERVER_MESSAGE__MSG_MATCH_GAME_RES) {
        LOG_ERROR("Invalid message type for match game response handler");
        return -1;
    }

    if (msg->match_game_res) {
        if (msg->match_game_res->success) {
            if (msg->match_game_res->assigned_team == TEAM__TEAM_UNSPECIFIED) {
                // 아직 매칭 대기 중
                LOG_INFO("Added to matchmaking queue. Game ID: %s",
                         msg->match_game_res->game_id ? msg->match_game_res->game_id : "unknown");
            } else {
                // 매칭 성공! 게임 시작
                LOG_INFO("Match found! Game ID: %s, Assigned team: %s",
                         msg->match_game_res->game_id ? msg->match_game_res->game_id : "unknown",
                         msg->match_game_res->assigned_team == TEAM__TEAM_WHITE ? "WHITE" : "BLACK");

                // 상대방 이름 확인
                const char *opponent_name = msg->match_game_res->opponent_name ? msg->match_game_res->opponent_name : "Opponent";
                LOG_INFO("Opponent name: %s", opponent_name);

                add_chat_message_safe("System", "Match found! Game starting...");

                // 클라이언트 상태 업데이트
                client_state_t *client = get_client_state();

                pthread_mutex_lock(&screen_mutex);

                strncpy(client->game_state.opponent_player, opponent_name, sizeof(client->game_state.opponent_player) - 1);
                client->game_state.opponent_player[sizeof(client->game_state.opponent_player) - 1] = '\0';

                strncpy(client->game_state.local_player, client->username, sizeof(client->game_state.local_player) - 1);
                client->game_state.local_player[sizeof(client->game_state.local_player) - 1] = '\0';

                // 프로토콜 team를 team_t로 변환 (team_WHITE=1 -> TEAM_WHITE=0, team_BLACK=2 -> TEAM_BLACK=1)
                if (msg->match_game_res->assigned_team == TEAM__TEAM_WHITE) {
                    client->game_state.local_team = TEAM_WHITE;
                } else if (msg->match_game_res->assigned_team == TEAM__TEAM_BLACK) {
                    client->game_state.local_team = TEAM_BLACK;
                } else {
                    LOG_ERROR("Invalid assigned team: %d", msg->match_game_res->assigned_team);
                    client->game_state.local_team = TEAM_WHITE;  // 기본값
                }
                client->game_state.game_in_progress = true;
                client->current_screen              = SCREEN_GAME;
                client->game_state.game_start_time  = time(NULL);

                // 게임 시작 시 보드 상태 초기화 (체스는 항상 WHITE가 먼저 시작)
                init_game(&client->game_state.game);
                // 플레이어 팀에 따라 보드를 적절히 배치
                reset_game_for_player(&client->game_state.game, client->game_state.local_team);
                client->game_state.game.side_to_move = TEAM_WHITE;

                // 타이머 정보 설정 (서버에서 받은 정보가 있으면 사용, 없으면 기본값)
                if (msg->match_game_res->time_limit_per_player > 0) {
                    client->game_state.white_time_remaining = msg->match_game_res->white_time_remaining;
                    client->game_state.black_time_remaining = msg->match_game_res->black_time_remaining;
                    LOG_INFO("Timer initialized: white=%d, black=%d",
                             client->game_state.white_time_remaining,
                             client->game_state.black_time_remaining);
                } else {
                    // 기본값 (10분)
                    client->game_state.white_time_remaining = DEFAULT_GAME_TIME_LIMIT;
                    client->game_state.black_time_remaining = DEFAULT_GAME_TIME_LIMIT;
                    LOG_INFO("Timer initialized with default values: %d seconds each", DEFAULT_GAME_TIME_LIMIT);
                }

                // 게임 시작 시간 설정 (서버에서 받은 시간이 있으면 사용)
                if (msg->match_game_res->game_start_time) {
                    client->game_state.game_start_time = msg->match_game_res->game_start_time->seconds;
                } else {
                    client->game_state.game_start_time = time(NULL);
                }

                // 실시간 타이머 필드 초기화
                client->game_state.last_timer_update       = client->game_state.game_start_time;
                client->game_state.white_time_at_last_sync = client->game_state.white_time_remaining;
                client->game_state.black_time_at_last_sync = client->game_state.black_time_remaining;

                // PGN 기록 초기화
                client->game_state.pgn_move_count = 0;
                strcpy(client->game_state.pgn_result, "*");

                // 네트워크 스레드에서 상태가 변경되었음을 메인 루프에 알림
                client->screen_update_requested = true;

                pthread_mutex_unlock(&screen_mutex);

                LOG_INFO("Game state updated: screen=%d, local_team=%d, assigned_team=%d, opponent=%s",
                         client->current_screen, client->game_state.local_team, msg->match_game_res->assigned_team, opponent_name);
            }
        } else {
            LOG_WARN("Matchmaking failed: %s",
                     msg->match_game_res->message ? msg->match_game_res->message : "no reason provided");

            add_chat_message_safe("System", msg->match_game_res->message ? msg->match_game_res->message : "Matchmaking failed");
            client_state_t *client = get_client_state();
            client->current_screen = SCREEN_MAIN;
        }
    } else {
        LOG_ERROR("Received match game response but no response data");
        add_chat_message_safe("System", "Invalid server response");
    }

    return 0;
}