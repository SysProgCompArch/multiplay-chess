#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../match_manager.h"
#include "../server_network.h"
#include "handlers.h"
#include "logger.h"

// 기권 메시지 처리
int handle_resign_message(int fd, ClientMessage *req) {
    if (!req || !req->resign) {
        LOG_ERROR("Invalid resign request from fd=%d", fd);
        return send_error_response(fd, 400, "Invalid resign request");
    }

    ResignRequest *resign_req = req->resign;
    LOG_INFO("Processing resign request from player: %s (fd=%d)",
             resign_req->player_id ? resign_req->player_id : "unknown", fd);

    // 매치 매니저에서 해당 플레이어가 참여한 게임 찾기
    pthread_mutex_lock(&g_match_manager.mutex);

    ActiveGame *game                   = NULL;
    Team        winner_team            = TEAM__TEAM_UNSPECIFIED;
    int         opponent_fd            = -1;
    char        game_id[64]            = {0};
    char        opponent_player_id[64] = {0};

    // 활성 게임에서 해당 플레이어 찾기
    for (int i = 0; i < MAX_ACTIVE_GAMES; i++) {
        ActiveGame *current_game = &g_match_manager.active_games[i];
        if (current_game->is_active) {
            if (current_game->white_player_fd == fd) {
                game        = current_game;
                winner_team = TEAM__TEAM_BLACK;
                opponent_fd = current_game->black_player_fd;
                strcpy(game_id, current_game->game_id);
                strcpy(opponent_player_id, current_game->black_player_id);
                break;
            } else if (current_game->black_player_fd == fd) {
                game        = current_game;
                winner_team = TEAM__TEAM_WHITE;
                opponent_fd = current_game->white_player_fd;
                strcpy(game_id, current_game->game_id);
                strcpy(opponent_player_id, current_game->white_player_id);
                break;
            }
        }
    }

    if (!game) {
        pthread_mutex_unlock(&g_match_manager.mutex);
        LOG_WARN("Player %s (fd=%d) tried to resign but is not in any active game",
                 resign_req->player_id ? resign_req->player_id : "unknown", fd);
        return send_error_response(fd, 404, "You are not in any active game");
    }

    // 기권 성공 응답을 요청자에게 전송
    ServerMessage  resign_response = SERVER_MESSAGE__INIT;
    ResignResponse resign_res      = RESIGN_RESPONSE__INIT;

    resign_res.game_id   = game_id;
    resign_res.player_id = resign_req->player_id;
    resign_res.success   = true;
    resign_res.message   = "Resignation successful";

    resign_response.msg_case   = SERVER_MESSAGE__MSG_RESIGN_RES;
    resign_response.resign_res = &resign_res;

    if (send_server_message(fd, &resign_response) < 0) {
        LOG_WARN("Failed to send resign response to player (fd=%d)", fd);
    }

    // 기권 브로드캐스트를 양쪽 플레이어에게 전송
    ServerMessage   resign_broadcast_msg = SERVER_MESSAGE__INIT;
    ResignBroadcast resign_broadcast     = RESIGN_BROADCAST__INIT;

    resign_broadcast.game_id     = game_id;
    resign_broadcast.player_id   = resign_req->player_id;
    resign_broadcast.winner_team = winner_team;

    resign_broadcast_msg.msg_case         = SERVER_MESSAGE__MSG_RESIGN_BROADCAST;
    resign_broadcast_msg.resign_broadcast = &resign_broadcast;

    // 기권한 플레이어에게 전송
    if (send_server_message(fd, &resign_broadcast_msg) < 0) {
        LOG_WARN("Failed to send resign broadcast to resigning player (fd=%d)", fd);
    }

    // 상대방에게 전송
    if (opponent_fd >= 0) {
        if (send_server_message(opponent_fd, &resign_broadcast_msg) < 0) {
            LOG_WARN("Failed to send resign broadcast to opponent (fd=%d)", opponent_fd);
        }
    }

    // 게임 종료
    game->is_active = false;
    g_match_manager.active_game_count--;

    pthread_mutex_unlock(&g_match_manager.mutex);

    LOG_INFO("Game %s ended due to resignation by %s", game_id, resign_req->player_id);

    return 0;
}