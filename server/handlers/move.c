#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "handlers.h"
#include "logger.h"
#include "match_manager.h"
#include "rule.h"
#include "utils.h"

// 체스 좌표를 배열 인덱스로 변환 (예: "a1" -> (0, 0))
bool parse_chess_coordinate(const char *coord, int *x, int *y) {
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
void format_chess_coordinate(int x, int y, char *coord) {
    coord[0] = 'a' + x;
    coord[1] = '1' + y;
    coord[2] = '\0';
}

// 헬퍼 함수: 에러 응답 전송
int send_move_error(int fd, const char *game_id, const char *player_id, const char *error_msg) {
    ServerMessage response    = SERVER_MESSAGE__INIT;
    MoveResponse  move_result = MOVE_RESPONSE__INIT;

    move_result.game_id   = (char *)game_id;
    move_result.player_id = (char *)player_id;
    move_result.success   = false;
    move_result.message   = (char *)error_msg;

    response.msg_case = SERVER_MESSAGE__MSG_MOVE_RES;
    response.move_res = &move_result;

    return send_server_message(fd, &response);
}

// 헬퍼 함수: 성공 응답 전송
int send_move_success(int fd, const char *game_id, const char *player_id, const char *updated_fen) {
    ServerMessage response    = SERVER_MESSAGE__INIT;
    MoveResponse  move_result = MOVE_RESPONSE__INIT;

    move_result.game_id     = (char *)game_id;
    move_result.player_id   = (char *)player_id;
    move_result.success     = true;
    move_result.message     = "Move successful";
    move_result.updated_fen = (char *)updated_fen;

    response.msg_case = SERVER_MESSAGE__MSG_MOVE_RES;
    response.move_res = &move_result;

    return send_server_message(fd, &response);
}

// 헬퍼 함수: 이동 브로드캐스트
int broadcast_move(int opponent_fd, const char *game_id, const char *player_id,
                   const char *from, const char *to) {
    ServerMessage broadcast      = SERVER_MESSAGE__INIT;
    MoveBroadcast move_broadcast = MOVE_BROADCAST__INIT;

    move_broadcast.game_id   = (char *)game_id;
    move_broadcast.player_id = (char *)player_id;
    move_broadcast.from      = (char *)from;
    move_broadcast.to        = (char *)to;
    move_broadcast.promotion = PIECE_TYPE__PT_NONE;  // TODO: 프로모션 처리

    broadcast.msg_case       = SERVER_MESSAGE__MSG_MOVE_BROADCAST;
    broadcast.move_broadcast = &move_broadcast;

    return send_server_message(opponent_fd, &broadcast);
}

// 기물 옮김 요청 처리 핸들러
int handle_move_message(int fd, ClientMessage *req) {
    if (req->msg_case != CLIENT_MESSAGE__MSG_MOVE) {
        LOG_ERROR("Invalid message type for move handler from fd=%d", fd);
        return -1;
    }

    MoveRequest *move_req = req->move;
    if (!move_req) {
        LOG_ERROR("Move request is null from fd=%d", fd);
        return -1;
    }

    LOG_INFO("Processing move request from fd=%d: %s -> %s", fd, move_req->from, move_req->to);

    // 플레이어가 속한 게임 찾기
    ActiveGame *game = find_game_by_player_fd(fd);
    if (!game) {
        LOG_WARN("Player fd=%d is not in any active game", fd);
        return send_move_error(fd, "", "", "You are not in an active game");
    }

    // 플레이어 ID 확인
    char *player_id    = NULL;
    int   opponent_fd  = -1;
    int   player_color = -1;

    if (game->white_player_fd == fd) {
        player_id    = game->white_player_id;
        opponent_fd  = game->black_player_fd;
        player_color = WHITE;
    } else if (game->black_player_fd == fd) {
        player_id    = game->black_player_id;
        opponent_fd  = game->white_player_fd;
        player_color = BLACK;
    } else {
        LOG_ERROR("Player fd=%d found in game but not as white or black player", fd);
        return send_move_error(fd, game->game_id, "", "Internal error: player not found in game");
    }

    // 턴 확인
    if (game->game_state.side_to_move != player_color) {
        LOG_WARN("Player fd=%d tried to move but it's not their turn", fd);
        return send_move_error(fd, game->game_id, player_id, "It's not your turn");
    }

    // 체스 좌표 파싱
    int from_x, from_y, to_x, to_y;
    if (!parse_chess_coordinate(move_req->from, &from_x, &from_y)) {
        LOG_WARN("Invalid 'from' coordinate: %s", move_req->from);
        return send_move_error(fd, game->game_id, player_id, "Invalid source coordinate");
    }

    if (!parse_chess_coordinate(move_req->to, &to_x, &to_y)) {
        LOG_WARN("Invalid 'to' coordinate: %s", move_req->to);
        return send_move_error(fd, game->game_id, player_id, "Invalid destination coordinate");
    }

    // 이동 가능성 검증
    // rule.c의 is_move_legal에서 sx=file(x), sy=rank(y)로 해석되므로
    // 체스 좌표를 (file, rank) = (x, y) 순서로 전달
    if (!is_move_legal(&game->game_state, from_x, from_y, to_x, to_y)) {
        LOG_WARN("Illegal move from fd=%d: %s -> %s", fd, move_req->from, move_req->to);
        return send_move_error(fd, game->game_id, player_id, "Illegal move");
    }

    // 이동 적용
    // apply_move 함수도 동일한 좌표 순서 사용
    apply_move(&game->game_state, from_x, from_y, to_x, to_y);

    LOG_INFO("Move applied successfully for fd=%d: %s -> %s", fd, move_req->from, move_req->to);

    // TODO: FEN 문자열 생성 (현재는 간단한 메시지로 대체)
    char fen_placeholder[256];
    snprintf(fen_placeholder, sizeof(fen_placeholder), "move_%d_to_%d",
             game->game_state.halfmove_clock, game->game_state.side_to_move);

    // 성공 응답 전송
    if (send_move_success(fd, game->game_id, player_id, fen_placeholder) < 0) {
        LOG_ERROR("Failed to send move success response to fd=%d", fd);
        return -1;
    }

    // 상대방에게 이동 브로드캐스트
    if (broadcast_move(opponent_fd, game->game_id, player_id,
                       move_req->from, move_req->to) < 0) {
        LOG_ERROR("Failed to broadcast move to opponent fd=%d", opponent_fd);
        // 이미 이동은 적용되었으므로, 브로드캐스트 실패만 로그하고 계속 진행
    }

    // 요청자에게도 이동 브로드캐스트
    if (broadcast_move(fd, game->game_id, player_id,
                       move_req->from, move_req->to) < 0) {
        LOG_ERROR("Failed to broadcast move to requester fd=%d", fd);
        // 이미 이동은 적용되었으므로, 브로드캐스트 실패만 로그하고 계속 진행
    }

    // 게임 종료 조건 확인 (체크메이트, 스테일메이트 등)
    if (is_checkmate(&game->game_state)) {
        LOG_INFO("Game %s ended by checkmate", game->game_id);
        // TODO: 게임 종료 처리 및 브로드캐스트
    } else if (is_stalemate(&game->game_state)) {
        LOG_INFO("Game %s ended by stalemate", game->game_id);
        // TODO: 게임 종료 처리 및 브로드캐스트
    } else if (is_fifty_move_rule(&game->game_state)) {
        LOG_INFO("Game %s ended by fifty-move rule", game->game_id);
        // TODO: 게임 종료 처리 및 브로드캐스트
    }

    return 0;
}