#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "../client_state.h"
#include "../game_state.h"
#include "handlers.h"
#include "logger.h"
#include "pgn.h"

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
                add_chat_message_safe("System", "Waiting for opponent...");
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

                 // ─── PGN 헤더 태그 설정 ─────────────────────────────
                {
                    PGNGame *pgn = &client->pgn;

                    // 날짜 태그
                    char date_str[16];
                    time_t now = time(NULL);
                    struct tm *tm = localtime(&now);
                    strftime(date_str, sizeof(date_str), "%Y.%m.%d", tm);
                    free(pgn->date);
                    pgn->date = strdup(date_str);

                    // 라운드 태그: 서버에서 받은 값이 없으므로 "1" 고정
                    free(pgn->round);
                    pgn->round = strdup("1");

                    // White/Black 태그
                    free(pgn->white);
                    free(pgn->black);
                    const char *opp = msg->match_game_res->opponent_name
                                    ? msg->match_game_res->opponent_name
                                    : "Opponent";
                    if (client->game_state.local_team == TEAM_WHITE) {
                        pgn->white = strdup(client->username);
                        pgn->black = strdup(opp);
                    } else {
                        pgn->white = strdup(opp);
                        pgn->black = strdup(client->username);
                    }
                }
                // ────────────────────────────────────────────────────


                // 게임 시작 시 보드 상태 초기화 (체스는 항상 WHITE가 먼저 시작)
                init_game(&client->game_state.game);
                // 플레이어 팀에 따라 보드를 적절히 배치
                reset_game_for_player(&client->game_state.game, client->game_state.local_team);
                client->game_state.game.side_to_move = TEAM_WHITE;

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