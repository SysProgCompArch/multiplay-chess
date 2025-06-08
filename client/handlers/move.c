#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "../client_state.h"
#include "../game_state.h"  // apply_move_from_server 함수를 위해 추가
#include "../ui/ui.h"       // show_dialog 함수를 위해 추가
#include "handlers.h"
#include "logger.h"

// 체스 좌표를 배열 인덱스로 변환 (예: "a1" -> (0, 0))
bool parse_chess_coordinate_client(const char *coord, int *x, int *y) {
    if (!coord || strlen(coord) != 2) {
        return false;
    }

    char file = coord[0];  // a-h
    char rank = coord[1];  // 1-8

    if (file < 'a' || file > 'h' || rank < '1' || rank > '8') {
        return false;
    }

    *x = file - 'a';  // 0-7
    *y = rank - '1';  // 0-7

    return true;
}

// 배열 인덱스를 체스 좌표로 변환 (예: (0, 0) -> "a1")
void format_chess_coordinate_client(int x, int y, char *coord) {
    coord[0] = 'a' + x;
    coord[1] = '1' + y;
    coord[2] = '\0';
}

// 서버로부터 받은 이동 결과 처리
int handle_move_response(ServerMessage *msg) {
    if (!msg || msg->msg_case != SERVER_MESSAGE__MSG_MOVE_RES) {
        LOG_ERROR("Invalid move result message");
        return -1;
    }

    MoveResponse *result = msg->move_res;
    if (!result) {
        LOG_ERROR("Move result is null");
        return -1;
    }

    LOG_INFO("Received move result: success=%s, message=%s",
             result->success ? "true" : "false",
             result->message ? result->message : "");

    if (result->success) {
        // 성공적인 이동
        char chat_msg[256];
        snprintf(chat_msg, sizeof(chat_msg), "Move successful: %s",
                 result->message ? result->message : "");
        add_chat_message_safe("System", chat_msg);

        // TODO: 게임 상태 업데이트 (FEN 파싱 등)
        if (result->updated_fen) {
            LOG_DEBUG("Updated FEN: %s", result->updated_fen);
        }

        // 화면 업데이트 요청
        client_state_t *client = get_client_state();
        pthread_mutex_lock(&screen_mutex);
        client->screen_update_requested = true;
        pthread_mutex_unlock(&screen_mutex);

        // UI는 자동으로 새로고침됩니다

    } else {
        // 실패한 이동
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Move failed: %s",
                 result->message ? result->message : "Unknown error");
        add_chat_message_safe("System", error_msg);

        LOG_WARN("Move failed: %s", result->message ? result->message : "Unknown error");
    }

    return 0;
}

// 상대방의 이동 브로드캐스트 처리
int handle_move_broadcast(ServerMessage *msg) {
    if (!msg || msg->msg_case != SERVER_MESSAGE__MSG_MOVE_BROADCAST) {
        LOG_ERROR("Invalid move broadcast message");
        return -1;
    }

    MoveBroadcast *broadcast = msg->move_broadcast;
    if (!broadcast) {
        LOG_ERROR("Move broadcast is null");
        return -1;
    }

    LOG_INFO("Received move broadcast: %s moved %s -> %s",
             broadcast->player_id ? broadcast->player_id : "Player",
             broadcast->from ? broadcast->from : "?",
             broadcast->to ? broadcast->to : "?");

    // 채팅에 이동 알림 추가
    char chat_msg[256];
    snprintf(chat_msg, sizeof(chat_msg), "%s moved %s -> %s",
             broadcast->player_id ? broadcast->player_id : "Opponent",
             broadcast->from ? broadcast->from : "?",
             broadcast->to ? broadcast->to : "?");
    add_chat_message_safe("Game", chat_msg);

    // 게임 보드 상태 업데이트
    if (broadcast->from && broadcast->to) {
        client_state_t *client = get_client_state();

        LOG_DEBUG("Applying move from server: %s -> %s", broadcast->from, broadcast->to);

        if (apply_move_from_server(&client->game_state.game, broadcast->from, broadcast->to)) {
            LOG_DEBUG("Board updated successfully: %s -> %s", broadcast->from, broadcast->to);

            pthread_mutex_lock(&screen_mutex);

            // 체크 상태 업데이트
            if (broadcast->is_check) {
                if (broadcast->checked_team == TEAM__TEAM_WHITE) {
                    client->game_state.white_in_check = true;
                    client->game_state.black_in_check = false;
                    LOG_INFO("White is in check");
                } else if (broadcast->checked_team == TEAM__TEAM_BLACK) {
                    client->game_state.white_in_check = false;
                    client->game_state.black_in_check = true;
                    LOG_INFO("Black is in check");
                }
            } else {
                // 체크 상태 해제
                client->game_state.white_in_check = false;
                client->game_state.black_in_check = false;
            }

            client->screen_update_requested = true;
            pthread_mutex_unlock(&screen_mutex);

        } else {
            LOG_ERROR("Failed to update board: %s -> %s (coordinates may be invalid)", broadcast->from, broadcast->to);
            add_chat_message_safe("System", "Failed to update board state - invalid move coordinates");
        }
    } else {
        LOG_WARN("Move broadcast missing coordinates: from=%s, to=%s",
                 broadcast->from ? broadcast->from : "NULL",
                 broadcast->to ? broadcast->to : "NULL");
        add_chat_message_safe("System", "Invalid move data received from server");
    }

    // 프로모션 처리
    if (broadcast->promotion != PIECE_TYPE__PT_NONE) {
        LOG_INFO("Piece promoted to: %d", broadcast->promotion);
        // TODO: 프로모션 처리
    }

    // 게임 종료 처리
    if (broadcast->game_ends) {
        LOG_INFO("Game ended: type=%d, winner=%d", broadcast->end_type, broadcast->winner_team);

        char        end_msg[256];
        const char *winner_str   = "";
        const char *end_type_str = "";

        // 승자 결정
        if (broadcast->winner_team == TEAM__TEAM_WHITE) {
            winner_str = "White wins";
        } else if (broadcast->winner_team == TEAM__TEAM_BLACK) {
            winner_str = "Black wins";
        } else {
            winner_str = "Draw";
        }

        // 게임 종료 유형 결정
        switch (broadcast->end_type) {
            case GAME_END_TYPE__GAME_END_CHECKMATE:
                end_type_str = "checkmate";
                break;
            case GAME_END_TYPE__GAME_END_STALEMATE:
                end_type_str = "stalemate";
                break;
            case GAME_END_TYPE__GAME_END_DRAW:
                end_type_str = "draw";
                break;
            case GAME_END_TYPE__GAME_END_RESIGN:
                end_type_str = "resignation";
                break;
            case GAME_END_TYPE__GAME_END_DISCONNECT:
                end_type_str = "disconnection";
                break;
            case GAME_END_TYPE__GAME_END_TIMEOUT:
                end_type_str = "timeout";
                break;
            default:
                end_type_str = "unknown reason";
                break;
        }

        snprintf(end_msg, sizeof(end_msg), "Game Over: %s by %s", winner_str, end_type_str);
        add_chat_message_safe("Game", end_msg);

        // 게임 상태를 종료 상태로 설정 및 다이얼로그 플래그 설정
        client_state_t *client = get_client_state();
        pthread_mutex_lock(&screen_mutex);
        client->game_state.game_in_progress = false;
        client->game_end_dialog_pending     = true;
        strncpy(client->game_end_message, end_msg, sizeof(client->game_end_message) - 1);
        client->screen_update_requested = true;
        pthread_mutex_unlock(&screen_mutex);
    }

    // UI는 자동으로 새로고침됩니다

    return 0;
}