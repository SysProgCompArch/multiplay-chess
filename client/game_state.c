#include "game_state.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "logger.h"  // LOG_DEBUG 함수를 위해 추가
#include "piece.h"
#include "types.h"
#include "utils.h"  // clear_board, fen_parse 함수를 위해 추가

// piece_table 외부 선언 (common/piece.c에 정의됨)
extern piece_t *piece_table[2][6];

// 스레드 동기화 (외부에서 정의됨)
extern pthread_mutex_t screen_mutex;

// 게임 상태 초기화
void init_game_state(game_state_t *state) {
    memset(state, 0, sizeof(game_state_t));

    // game_t 초기화
    init_game(&state->game);

    state->chat_count            = 0;
    state->local_team            = TEAM_WHITE;
    state->game_in_progress      = false;
    state->game_start_time       = 0;
    state->white_time_remaining  = 600;  // 10분
    state->black_time_remaining  = 600;  // 10분
    state->opponent_disconnected = false;

    // 체크 상태 초기화
    state->white_in_check = false;
    state->black_in_check = false;

    // 이동 가능 위치 배열 초기화
    memset(state->possible_moves, false, sizeof(state->possible_moves));
    memset(state->capture_moves, false, sizeof(state->capture_moves));
}

void init_game(game_t *game) {
    memset(game, 0, sizeof(game_t));

    // 보드 초기화
    clear_board(game);

    // 시작 위치로 설정
    reset_game_to_starting_position(game);

    // 게임 상태 초기화
    game->side_to_move    = TEAM_WHITE;
    game->halfmove_clock  = 0;
    game->fullmove_number = 1;

    // 캐슬링 권리 초기화
    game->white_can_castle_kingside  = true;
    game->white_can_castle_queenside = true;
    game->black_can_castle_kingside  = true;
    game->black_can_castle_queenside = true;

    // 앙파상 초기화
    game->en_passant_x = -1;
    game->en_passant_y = -1;
}

void reset_game_to_starting_position(game_t *game) {
    // FEN 문자열로 시작 위치 설정
    const char *starting_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    fen_parse(game, starting_fen);
}

// 플레이어 팀에 따라 보드를 적절히 초기화
void reset_game_for_player(game_t *game, team_t player_team) {
    // 플레이어 팀에 관계없이 항상 표준 FEN으로 초기화
    // UI에서 플레이어 관점에 따라 표시를 뒤집어서 보여줌
    const char *starting_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    fen_parse(game, starting_fen);
}

// 채팅 메시지 추가
void add_chat_message(game_state_t *state, const char *sender, const char *message) {
    if (state->chat_count >= MAX_CHAT_MESSAGES) {
        // 배열이 가득 찬 경우 가장 오래된 메시지 제거
        for (int i = 1; i < MAX_CHAT_MESSAGES; i++) {
            state->chat_messages[i - 1] = state->chat_messages[i];
        }
        state->chat_count = MAX_CHAT_MESSAGES - 1;
    }

    chat_message_t *msg = &state->chat_messages[state->chat_count];
    strncpy(msg->sender, sender, sizeof(msg->sender) - 1);
    msg->sender[sizeof(msg->sender) - 1] = '\0';

    strncpy(msg->message, message, sizeof(msg->message) - 1);
    msg->message[sizeof(msg->message) - 1] = '\0';

    msg->timestamp = time(NULL);
    state->chat_count++;
}

// 유효한 수인지 확인 (기본 체크)
bool is_valid_move(game_t *game, int from_x, int from_y, int to_x, int to_y) {
    return is_move_legal(game, from_x, from_y, to_x, to_y);
}

// 실제 수 실행
bool make_move(game_t *game, int from_x, int from_y, int to_x, int to_y) {
    if (!is_valid_move(game, from_x, from_y, to_x, to_y)) {
        return false;
    }

    apply_move(game, from_x, from_y, to_x, to_y);
    return true;
}

// 체크 상태 확인 (클라이언트용 래퍼 함수)
bool is_in_check_client(game_t *game, team_t team) {
    return is_in_check(game, team);
}

// 체크메이트 확인 (클라이언트용 래퍼 함수)
bool is_checkmate_client(game_t *game, team_t team) {
    return is_checkmate(game);
}

// 스테일메이트 확인 (클라이언트용 래퍼 함수)
bool is_stalemate_client(game_t *game, team_t team) {
    return is_stalemate(game);
}

// 편의 함수들 (클라이언트 상태 호환성)
bool game_is_white_player(const game_state_t *state) {
    return state->local_team == TEAM_WHITE;
}

const char *get_opponent_name(const game_state_t *state) {
    return state->opponent_player;
}

// 서버로부터 받은 이동을 보드에 적용
bool apply_move_from_server(game_t *game, const char *from, const char *to) {
    // NULL 포인터와 빈 문자열 검사
    if (!game || !from || !to || strlen(from) != 2 || strlen(to) != 2) {
        return false;
    }

    // 체스 표기법을 좌표로 변환
    // 체스 표기법: "a1" = file 'a' (0-7), rank '1' (0-7)
    // apply_move 함수는 (x, y) 형식으로 받으므로 file을 x, rank를 y로 변환
    int from_x = from[0] - 'a';  // file (a-h) -> x (0-7)
    int from_y = from[1] - '1';  // rank (1-8) -> y (0-7)
    int to_x   = to[0] - 'a';    // file (a-h) -> x (0-7)
    int to_y   = to[1] - '1';    // rank (1-8) -> y (0-7)

    // 범위 검사
    if (from_x < 0 || from_x >= 8 || from_y < 0 || from_y >= 8 ||
        to_x < 0 || to_x >= 8 || to_y < 0 || to_y >= 8) {
        return false;
    }

    LOG_DEBUG("=== MOVE DEBUG START ===");
    LOG_DEBUG("Applying server move: %s (%d,%d) -> %s (%d,%d)",
              from, from_x, from_y, to, to_x, to_y);

    // 이동 전 보드 상태 확인 (두 가지 접근 방식으로 확인)
    piecestate_t *src_xy = &game->board[from_x][from_y];  // [x][y] 접근
    piecestate_t *src_yx = &game->board[from_y][from_x];  // [y][x] 접근

    LOG_DEBUG("Source [%d][%d] (x,y): piece=%p, dead=%d",
              from_x, from_y, (void *)src_xy->piece, src_xy->is_dead);
    LOG_DEBUG("Source [%d][%d] (y,x): piece=%p, dead=%d",
              from_y, from_x, (void *)src_yx->piece, src_yx->is_dead);

    piecestate_t *dst_xy = &game->board[to_x][to_y];  // [x][y] 접근
    piecestate_t *dst_yx = &game->board[to_y][to_x];  // [y][x] 접근

    LOG_DEBUG("Target [%d][%d] (x,y): piece=%p, dead=%d",
              to_x, to_y, (void *)dst_xy->piece, dst_xy->is_dead);
    LOG_DEBUG("Target [%d][%d] (y,x): piece=%p, dead=%d",
              to_y, to_x, (void *)dst_yx->piece, dst_yx->is_dead);

    // 서버로부터 받은 이동은 이미 검증된 유효한 이동이므로 검증 없이 직접 적용
    // (클라이언트 상태와 서버 상태 간의 불일치로 인한 검증 실패를 방지)
    //
    // rule.c 수정 후: apply_move 함수는 (file, rank) = (x, y) 순서로 받음
    apply_move(game, from_x, from_y, to_x, to_y);

    // 이동 후 보드 상태 확인
    LOG_DEBUG("After apply_move:");
    LOG_DEBUG("Source [%d][%d] (x,y): piece=%p, dead=%d",
              from_x, from_y, (void *)src_xy->piece, src_xy->is_dead);
    LOG_DEBUG("Target [%d][%d] (x,y): piece=%p, dead=%d",
              to_x, to_y, (void *)dst_xy->piece, dst_xy->is_dead);
    LOG_DEBUG("=== MOVE DEBUG END ===");

    return true;
}