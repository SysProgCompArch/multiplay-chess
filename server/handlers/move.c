#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../match_manager.h"
#include "handlers.h"
#include "logger.h"
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

// 헬퍼 함수: 이동 브로드캐스트 (게임 상태 정보 포함)
int broadcast_move_with_state(int fd, const char *game_id, const char *player_id,
                              const char *from, const char *to,
                              bool game_ends, Team winner_team, GameEndType end_type,
                              bool is_check, Team checked_team,
                              int32_t white_time_remaining, int32_t black_time_remaining) {
    ServerMessage broadcast      = SERVER_MESSAGE__INIT;
    MoveBroadcast move_broadcast = MOVE_BROADCAST__INIT;

    move_broadcast.game_id      = (char *)game_id;
    move_broadcast.player_id    = (char *)player_id;
    move_broadcast.from         = (char *)from;
    move_broadcast.to           = (char *)to;
    move_broadcast.promotion    = PIECE_TYPE__PT_NONE;  // TODO: 프로모션 처리
    move_broadcast.game_ends    = game_ends;
    move_broadcast.winner_team  = winner_team;
    move_broadcast.end_type     = end_type;
    move_broadcast.is_check     = is_check;
    move_broadcast.checked_team = checked_team;

    // 타이머 정보 추가
    move_broadcast.white_time_remaining = white_time_remaining;
    move_broadcast.black_time_remaining = black_time_remaining;

    // 이동 시점 타임스탬프 설정
    move_broadcast.move_timestamp = malloc(sizeof(Google__Protobuf__Timestamp));
    if (move_broadcast.move_timestamp) {
        google__protobuf__timestamp__init(move_broadcast.move_timestamp);
        move_broadcast.move_timestamp->seconds = time(NULL);
        move_broadcast.move_timestamp->nanos   = 0;
    }

    broadcast.msg_case       = SERVER_MESSAGE__MSG_MOVE_BROADCAST;
    broadcast.move_broadcast = &move_broadcast;

    int result = send_server_message(fd, &broadcast);

    // 메모리 해제
    if (move_broadcast.move_timestamp) {
        free(move_broadcast.move_timestamp);
    }

    return result;
}

// 헬퍼 함수: 이동 브로드캐스트 (하위 호환성을 위한 간단 버전)
int broadcast_move(int fd, const char *game_id, const char *player_id,
                   const char *from, const char *to) {
    return broadcast_move_with_state(fd, game_id, player_id, from, to,
                                     false, TEAM__TEAM_UNSPECIFIED, GAME_END_TYPE__GAME_END_UNKNOWN,
                                     false, TEAM__TEAM_UNSPECIFIED, 0, 0);
}

// 헬퍼 함수: 게임 종료 브로드캐스트
int send_game_end_broadcast(ActiveGame *game, const char *player_id, Team winner_team, GameEndType end_type) {
    ServerMessage    game_end_msg       = SERVER_MESSAGE__INIT;
    GameEndBroadcast game_end_broadcast = GAME_END_BROADCAST__INIT;

    game_end_broadcast.game_id     = game->game_id;
    game_end_broadcast.player_id   = (char *)player_id;
    game_end_broadcast.winner_team = winner_team;
    game_end_broadcast.end_type    = end_type;
    // timestamp는 NULL로 두면 protobuf에서 자동으로 처리

    game_end_msg.msg_case = SERVER_MESSAGE__MSG_GAME_END;
    game_end_msg.game_end = &game_end_broadcast;

    // 양쪽 플레이어 모두에게 게임 종료 브로드캐스트 전송
    int result = 0;
    if (send_server_message(game->white_player_fd, &game_end_msg) < 0) {
        LOG_ERROR("Failed to send game end broadcast to white player fd=%d", game->white_player_fd);
        result = -1;
    }
    if (send_server_message(game->black_player_fd, &game_end_msg) < 0) {
        LOG_ERROR("Failed to send game end broadcast to black player fd=%d", game->black_player_fd);
        result = -1;
    }

    LOG_INFO("Sent game end broadcast for game %s (end_type=%d, winner_team=%d)",
             game->game_id, end_type, winner_team);

    return result;
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
    char *player_id   = NULL;
    int   opponent_fd = -1;
    int   player_team = -1;

    if (game->white_player_fd == fd) {
        player_id   = game->white_player_id;
        opponent_fd = game->black_player_fd;
        player_team = WHITE;
    } else if (game->black_player_fd == fd) {
        player_id   = game->black_player_id;
        opponent_fd = game->white_player_fd;
        player_team = BLACK;
    } else {
        LOG_ERROR("Player fd=%d found in game but not as white or black player", fd);
        return send_move_error(fd, game->game_id, "", "Internal error: player not found in game");
    }

    // 턴 확인
    if (game->game_state.side_to_move != player_team) {
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

    // 타이머 업데이트 - 밀리초 단위로 정밀하게 관리
    // 타이머 스레드가 시간 차감을 담당하므로 여기서는 시간 기록만 업데이트
    extern MatchManager g_match_manager;
    extern int64_t      get_current_time_ms(void);  // 함수 선언

    pthread_mutex_lock(&g_match_manager.mutex);

    // 다음 턴을 위해 마지막 이동 시간만 업데이트 (밀리초 단위)
    // 시간 차감은 타이머 스레드가 지속적으로 처리하고 있음
    game->last_move_time_ms = get_current_time_ms();

    // last_timer_check_ms는 타이머 스레드에서만 업데이트하도록 함
    // 이렇게 하면 타이머 스레드가 정확한 경과 시간을 계산할 수 있음

    pthread_mutex_unlock(&g_match_manager.mutex);

    LOG_DEBUG("Timer updated for game %s: white=%d, black=%d",
              game->game_id, game->white_time_remaining, game->black_time_remaining);

    // TODO: FEN 문자열 생성 (현재는 간단한 메시지로 대체)
    char fen_placeholder[256];
    snprintf(fen_placeholder, sizeof(fen_placeholder), "move_%d_to_%d",
             game->game_state.halfmove_clock, game->game_state.side_to_move);

    // 성공 응답 전송
    if (send_move_success(fd, game->game_id, player_id, fen_placeholder) < 0) {
        LOG_ERROR("Failed to send move success response to fd=%d", fd);
        return -1;
    }

    // 게임 상태 확인 (체크, 체크메이트, 스테일메이트 등)
    team_t      current_side       = game->game_state.side_to_move;  // 이동 후 현재 턴 (상대방)
    bool        is_check_situation = is_in_check(&game->game_state, current_side);
    bool        game_ends          = false;
    Team        winner_team        = TEAM__TEAM_UNSPECIFIED;
    GameEndType end_type           = GAME_END_TYPE__GAME_END_UNKNOWN;
    Team        checked_team       = TEAM__TEAM_UNSPECIFIED;

    if (is_check_situation) {
        checked_team = (current_side == TEAM_WHITE) ? TEAM__TEAM_WHITE : TEAM__TEAM_BLACK;
        LOG_INFO("Check detected for %s in game %s",
                 (current_side == TEAM_WHITE) ? "WHITE" : "BLACK", game->game_id);
    }

    // 게임 종료 조건 확인 (시간 초과는 타이머 스레드에서 처리)
    if (is_checkmate(&game->game_state)) {
        LOG_INFO("Game %s ended by checkmate", game->game_id);
        game_ends   = true;
        winner_team = (current_side == TEAM_WHITE) ? TEAM__TEAM_BLACK : TEAM__TEAM_WHITE;
        end_type    = GAME_END_TYPE__GAME_END_CHECKMATE;
    } else if (is_stalemate(&game->game_state)) {
        LOG_INFO("Game %s ended by stalemate", game->game_id);
        game_ends   = true;
        winner_team = TEAM__TEAM_UNSPECIFIED;
        end_type    = GAME_END_TYPE__GAME_END_STALEMATE;
    } else if (is_fifty_move_rule(&game->game_state)) {
        LOG_INFO("Game %s ended by fifty-move rule", game->game_id);
        game_ends   = true;
        winner_team = TEAM__TEAM_UNSPECIFIED;
        end_type    = GAME_END_TYPE__GAME_END_DRAW;
    }

    // 상대방에게 이동 브로드캐스트 (게임 상태 정보 포함)
    // 밀리초를 초 단위로 변환해서 전송 (클라이언트 호환성 유지)
    if (broadcast_move_with_state(opponent_fd, game->game_id, player_id,
                                  move_req->from, move_req->to,
                                  game_ends, winner_team, end_type,
                                  is_check_situation, checked_team,
                                  game->white_time_remaining / 1000, game->black_time_remaining / 1000) < 0) {
        LOG_ERROR("Failed to broadcast move to opponent fd=%d", opponent_fd);
        // 이미 이동은 적용되었으므로, 브로드캐스트 실패만 로그하고 계속 진행
    }

    // 요청자에게도 이동 브로드캐스트 (게임 상태 정보 포함)
    // 밀리초를 초 단위로 변환해서 전송 (클라이언트 호환성 유지)
    if (broadcast_move_with_state(fd, game->game_id, player_id,
                                  move_req->from, move_req->to,
                                  game_ends, winner_team, end_type,
                                  is_check_situation, checked_team,
                                  game->white_time_remaining / 1000, game->black_time_remaining / 1000) < 0) {
        LOG_ERROR("Failed to broadcast move to requester fd=%d", fd);
        // 이미 이동은 적용되었으므로, 브로드캐스트 실패만 로그하고 계속 진행
    }

    // 게임이 종료된 경우 매치 매니저에서 제거
    if (game_ends) {
        remove_game(game->game_id);
    }

    return 0;
}