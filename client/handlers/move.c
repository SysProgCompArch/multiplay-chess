#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h> 

#include "../client_state.h"
#include "../game_state.h"  // apply_move_from_server 함수를 위해 추가
#include "../ui/ui.h"       // show_dialog 함수를 위해 추가
#include "handlers.h"
#include "logger.h"
#include "pgn.h"

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

    client_state_t *cli = get_client_state();
    game_t         *game = &cli->game_state.game;
    PGNGame        *pgn  = &cli->pgn;
    char            pchar = '\0';

    // 1) 좌표 변환 (예: "e2" -> fx=4, fy=1)
    int fx = broadcast->from[0] - 'a';
    int fy = broadcast->from[1] - '1';
    int tx = broadcast->to  [0] - 'a';
    int ty = broadcast->to  [1] - '1';

    // 2) 캐슬링 여부 미리 체크 (킹이 같은 랭크에서 두 칸 이동했는지)
    bool is_castle = false;
    int  rook_fx, rook_tx;
    piecestate_t *orig = &game->board[fy][fx];
    if (orig->piece && orig->piece->type == PIECE_KING
        && fy == ty && abs(tx - fx) == 2) {
        is_castle = true;
        if (tx > fx) {
            // 킹사이드 캐슬링 O-O
            rook_fx = 7;        // h-file
            rook_tx = tx - 1;   // g-file
        } else {
            // 퀸사이드 캐슬링 O-O-O
            rook_fx = 0;        // a-file
            rook_tx = tx + 1;   // d-file
        }
    }

    // 3) 킹(또는 정상 이동) 적용
    apply_move_from_server(game, broadcast->from, broadcast->to);

    // 4) 캐슬링인 경우 룩 이동
    if (is_castle) {
        char rf[3] = { (char)('a' + rook_fx), (char)('1' + fy), '\0' };
        char rt[3] = { (char)('a' + rook_tx), (char)('1' + ty), '\0' };
        apply_move_from_server(game, rf, rt);

        // 채팅 알림
        add_chat_message_safe("Game", (tx > fx) ? "O-O" : "O-O-O");
    }

    // 5) 이동 메시지
    char chat_msg[128];
    snprintf(chat_msg, sizeof(chat_msg),
             "%s moved %s -> %s",
             broadcast->player_id ? broadcast->player_id : "Opponent",
             broadcast->from, broadcast->to);
    add_chat_message_safe("Game", chat_msg);

    // 6) 프로모션 기호 결정
    if (broadcast->promotion != PIECE_TYPE__PT_NONE) {
        switch (broadcast->promotion) {
            case PIECE_TYPE__PT_QUEEN:  pchar = 'Q'; break;
            case PIECE_TYPE__PT_ROOK:   pchar = 'R'; break;
            case PIECE_TYPE__PT_BISHOP: pchar = 'B'; break;
            case PIECE_TYPE__PT_KNIGHT: pchar = 'N'; break;
            default:                    pchar = '?'; break;
        }
    }

    // 7) SAN 생성 및 PGN 기록
    char san_buf[8];
    if (pchar) {
        snprintf(san_buf, sizeof(san_buf), "%s=%c", broadcast->to, pchar);
    } else {
        snprintf(san_buf, sizeof(san_buf), "%s%s", broadcast->from, broadcast->to);
    }
    // moves 배열 확장
    if (pgn->move_capacity == 0) {
        pgn->move_capacity = 16;
        pgn->moves         = malloc(sizeof(PGNMove) * pgn->move_capacity);
    }
    if (pgn->move_count >= pgn->move_capacity) {
        size_t new_cap = pgn->move_capacity * 2;
        pgn->moves = realloc(pgn->moves, sizeof(PGNMove) * new_cap);
        pgn->move_capacity = new_cap;
    }
    pgn->moves[pgn->move_count++].san = strdup(san_buf);

    // 8) 체크 상태 업데이트
    pthread_mutex_lock(&screen_mutex);
    if (broadcast->is_check) {
        if (broadcast->checked_team == TEAM__TEAM_WHITE) {
            cli->game_state.white_in_check = true;
            cli->game_state.black_in_check = false;
        } else {
            cli->game_state.white_in_check = false;
            cli->game_state.black_in_check = true;
        }
    } else {
        cli->game_state.white_in_check = false;
        cli->game_state.black_in_check = false;
    }
    cli->screen_update_requested = true;
    pthread_mutex_unlock(&screen_mutex);

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

        // PGN 저장
        char filename[128];
        sprintf(filename, "replay/game_%s_vs_%s.pgn", client->username, get_opponent_name_client());
        if (save_pgn_file(filename, &client->pgn) == 0) {
            LOG_INFO("PGN saved to %s", filename);
        } else {
            LOG_ERROR("Failed to save PGN");
        }

        // 3) pgn_free으로 메모리 해제
        pgn_free(&client->pgn);
    }

    // UI는 자동으로 새로고침됩니다

    return 0;
}