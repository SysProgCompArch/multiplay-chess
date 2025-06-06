#include <stdio.h>
#include <string.h>

#include "../client_state.h"
#include "../game_state.h"  // apply_move_from_server 함수를 위해 추가
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
        if (apply_move_from_server(&client->game_state.board_state, broadcast->from, broadcast->to)) {
            LOG_DEBUG("Board updated successfully: %s -> %s", broadcast->from, broadcast->to);
        } else {
            LOG_ERROR("Failed to update board: %s -> %s", broadcast->from, broadcast->to);
            add_chat_message_safe("System", "Failed to update board state");
        }
    }

    // 프로모션 처리
    if (broadcast->promotion != PIECE_TYPE__PT_NONE) {
        LOG_INFO("Piece promoted to: %d", broadcast->promotion);
        // TODO: 프로모션 처리
    }

    // UI는 자동으로 새로고침됩니다

    return 0;
}