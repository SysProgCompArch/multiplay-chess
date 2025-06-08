// rule.c
#include "rule.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

// 보드 범위 체크
static inline bool in_board(int x, int y) {
    return x >= 0 && x < 8 && y >= 0 && y < 8;
}

// (x,y) 칸이 by_team 편에게 공격당하는지
static bool is_square_attacked(const game_t *G, int x, int y, team_t by_team) {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            const piecestate_t *ps = &G->board[j][i];
            // NULL 포인터 검사 추가 - ps->piece가 NULL일 수 있음
            if (ps->is_dead || ps->piece == NULL || ps->team != by_team)
                continue;

            switch (ps->piece->type) {
                case PIECE_PAWN: {
                    int dir = (by_team == TEAM_WHITE ? +1 : -1);
                    if (j + dir == y && (i - 1 == x || i + 1 == x))
                        return true;
                    break;
                }
                case PIECE_KNIGHT:
                case PIECE_KING: {
                    for (int k = 0; k < ps->piece->offset_len; k++) {
                        offset_t o = ps->piece->offsets[k];
                        if (i + o.x == x && j + o.y == y)
                            return true;
                    }
                    break;
                }
                case PIECE_BISHOP:
                case PIECE_ROOK:
                case PIECE_QUEEN: {
                    for (int k = 0; k < ps->piece->offset_len; k++) {
                        offset_t o  = ps->piece->offsets[k];
                        int      nx = i + o.x, ny = j + o.y;
                        while (in_board(nx, ny)) {
                            if (nx == x && ny == y)
                                return true;
                            if (!G->board[ny][nx].is_dead)
                                break;
                            nx += o.x;
                            ny += o.y;
                        }
                    }
                    break;
                }
            }
        }
    }
    return false;
}

// 이동 규칙 검사
bool is_move_legal(const game_t *G, int sx, int sy, int dx, int dy) {
    if (!in_board(sx, sy) || !in_board(dx, dy))
        return false;
    const piecestate_t *src = &G->board[sy][sx];
    const piecestate_t *dst = &G->board[dy][dx];
    // NULL 포인터 검사 추가
    if (src->is_dead || src->piece == NULL || src->team != G->side_to_move)
        return false;
    if (!dst->is_dead && dst->piece != NULL && dst->team == G->side_to_move)
        return false;

    int  rx = dx - sx, ry = dy - sy;
    bool ok = false;

    switch (src->piece->type) {
        case PIECE_PAWN: {
            int dir = (src->team == TEAM_WHITE ? +1 : -1);
            // 한 칸 전진
            if (rx == 0 && ry == dir && dst->is_dead)
                ok = true;
            // 두 칸 전진
            if (rx == 0 && ry == 2 * dir && !src->has_moved && dst->is_dead && G->board[sy + dir][sx].is_dead)
                ok = true;
            // 대각선 캡처
            if (abs(rx) == 1 && ry == dir && (!dst->is_dead && dst->team != src->team || (dst->is_dead && G->en_passant_x == dx && G->en_passant_y == dy)))
                ok = true;
            break;
        }
        case PIECE_KNIGHT:
        case PIECE_KING: {
            for (int k = 0; k < src->piece->offset_len; k++) {
                offset_t o = src->piece->offsets[k];
                if (o.x == rx && o.y == ry) {
                    // 캐슬링 처리
                    if (src->piece->type == PIECE_KING && abs(rx) == 2 && ry == 0) {
                        bool ks  = rx > 0;
                        bool can = (src->team == TEAM_WHITE)
                                       ? (ks ? G->white_can_castle_kingside : G->white_can_castle_queenside)
                                       : (ks ? G->black_can_castle_kingside : G->black_can_castle_queenside);
                        if (!can)
                            return false;

                        int step = ks ? +1 : -1;
                        // 1) 중간 칸 비어있는지
                        for (int x = sx + step; x != dx; x += step)
                            if (!G->board[sy][x].is_dead)
                                return false;
                        // 2) 지나가는 칸(출발칸, 중간칸, 도착칸) 모두 공격받지 않는지
                        for (int x = sx; x != dx + step; x += step)
                            if (is_square_attacked(G, x, sy, 1 - src->team))
                                return false;
                    }
                    ok = true;
                    break;
                }
            }
            break;
        }
        case PIECE_BISHOP:
        case PIECE_ROOK:
        case PIECE_QUEEN: {
            for (int k = 0; k < src->piece->offset_len; k++) {
                offset_t o     = src->piece->offsets[k];
                int      dd    = o.x ? rx / o.x : ry / o.y;
                bool     align = (o.x == 0
                                      ? (rx == 0 && ry / o.y > 0)
                                      : (o.y == 0
                                             ? (ry == 0 && rx / o.x > 0)
                                             : (rx % o.x == 0 && ry % o.y == 0 && dd == ry / o.y && dd > 0)));
                if (!align)
                    continue;
                // 중간 칸 확인
                bool blocked = false;
                for (int s = 1; s < dd; s++) {
                    if (!G->board[sy + o.y * s][sx + o.x * s].is_dead) {
                        blocked = true;
                        break;
                    }
                }
                if (!blocked) {
                    ok = true;
                    break;
                }
            }
            break;
        }
    }

    if (!ok)
        return false;

    // 자기 장군 방지
    game_t tmp = *G;
    apply_move(&tmp, sx, sy, dx, dy);
    if (is_in_check(&tmp, src->team))
        return false;

    return true;
}

// 이동 적용
void apply_move(game_t *G, int sx, int sy, int dx, int dy) {
    piecestate_t *src = &G->board[sy][sx];
    piecestate_t *dst = &G->board[dy][dx];

    // NULL 포인터 검사 추가
    if (src->piece == NULL) {
        return;  // 소스 기물이 NULL이면 이동할 수 없음
    }

    // 50수 규칙 반영
    if (src->piece->type == PIECE_PAWN || !dst->is_dead)
        G->halfmove_clock = 0;
    else
        G->halfmove_clock++;

    // 앙파상 캡처
    if (src->piece->type == PIECE_PAWN && dx == G->en_passant_x && dy == G->en_passant_y) {
        piecestate_t *cap = &G->board[sy][dx];
        cap->is_dead      = 1;
        cap->piece        = NULL;
    }

    // 이동
    *dst         = *src;
    src->is_dead = 1;
    src->piece   = NULL;

    // 캐슬링 룩 이동
    if (dst->piece != NULL && dst->piece->type == PIECE_KING && abs(dx - sx) == 2) {
        bool         ks                   = dx > sx;
        int          rook_from_x          = ks ? 7 : 0;
        int          rook_to_x            = ks ? dx - 1 : dx + 1;
        piecestate_t rook                 = G->board[dy][rook_from_x];
        G->board[dy][rook_to_x]           = rook;
        G->board[dy][rook_from_x].is_dead = 1;
        G->board[dy][rook_from_x].piece   = NULL;
    }

    // 캐슬링 권리 소멸
    if (dst->piece != NULL && dst->piece->type == PIECE_KING) {
        if (dst->team == TEAM_WHITE)
            G->white_can_castle_kingside = G->white_can_castle_queenside = false;
        else
            G->black_can_castle_kingside = G->black_can_castle_queenside = false;
    }
    if (dst->piece != NULL && dst->piece->type == PIECE_ROOK && !dst->has_moved) {
        if (dst->team == TEAM_WHITE) {
            if (sx == 0 && sy == 0)
                G->white_can_castle_queenside = false;
            if (sx == 7 && sy == 0)
                G->white_can_castle_kingside = false;
        } else {
            if (sx == 0 && sy == 7)
                G->black_can_castle_queenside = false;
            if (sx == 7 && sy == 7)
                G->black_can_castle_kingside = false;
        }
    }

    // 앙파상 타겟 갱신
    if (dst->piece != NULL && dst->piece->type == PIECE_PAWN && abs(dy - sy) == 2) {
        G->en_passant_x = dx;
        G->en_passant_y = (sy + dy) / 2;
    } else {
        G->en_passant_x = G->en_passant_y = -1;
    }

    // 프로모션 (기본 퀸으로 자동 승격)
    if (dst->piece != NULL && dst->piece->type == PIECE_PAWN && (dy == 7 || dy == 0)) {
        // extern으로 정의된 queen_piecestate를 가리키도록 하거나
        // 별도의 get_promotion_piece(team) 함수를 호출하세요.
        dst->piece = get_default_queen(dst->team);
    }

    dst->has_moved  = true;
    G->side_to_move = (G->side_to_move == TEAM_WHITE) ? TEAM_BLACK : TEAM_WHITE;
}

// 체크 상태 확인
bool is_in_check(const game_t *G, team_t team) {
    // 킹의 위치 찾기
    int king_x = -1, king_y = -1;
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            const piecestate_t *ps = &G->board[y][x];
            // NULL 포인터 검사 추가 - ps->piece가 NULL일 수 있음
            if (!ps->is_dead && ps->piece != NULL && ps->team == team && ps->piece->type == PIECE_KING) {
                king_x = x;
                king_y = y;
                break;
            }
        }
        if (king_x != -1) break;
    }

    if (king_x == -1) return false;  // 킹이 없으면 체크가 아님

    return is_square_attacked(G, king_x, king_y, (team == TEAM_WHITE) ? TEAM_BLACK : TEAM_WHITE);
}

// 체크메이트 확인
bool is_checkmate(const game_t *G) {
    team_t team = G->side_to_move;

    // 체크 상태가 아니면 체크메이트가 아님
    if (!is_in_check(G, team)) {
        return false;
    }

    // 모든 가능한 수를 시도해서 체크에서 벗어날 수 있는지 확인
    for (int sx = 0; sx < 8; sx++) {
        for (int sy = 0; sy < 8; sy++) {
            const piecestate_t *src = &G->board[sy][sx];
            if (src->is_dead || src->piece == NULL || src->team != team) continue;

            for (int dx = 0; dx < 8; dx++) {
                for (int dy = 0; dy < 8; dy++) {
                    if (is_move_legal(G, sx, sy, dx, dy)) {
                        return false;  // 합법적인 수가 있으면 체크메이트가 아님
                    }
                }
            }
        }
    }

    return true;  // 합법적인 수가 없으면 체크메이트
}

// 스테일메이트 확인
bool is_stalemate(const game_t *G) {
    team_t team = G->side_to_move;

    // 체크 상태이면 스테일메이트가 아님
    if (is_in_check(G, team)) {
        return false;
    }

    // 모든 가능한 수를 시도해서 합법적인 수가 있는지 확인
    for (int sx = 0; sx < 8; sx++) {
        for (int sy = 0; sy < 8; sy++) {
            const piecestate_t *src = &G->board[sy][sx];
            if (src->is_dead || src->piece == NULL || src->team != team) continue;

            for (int dx = 0; dx < 8; dx++) {
                for (int dy = 0; dy < 8; dy++) {
                    if (is_move_legal(G, sx, sy, dx, dy)) {
                        return false;  // 합법적인 수가 있으면 스테일메이트가 아님
                    }
                }
            }
        }
    }

    return true;  // 합법적인 수가 없으면 스테일메이트
}

// 50수 규칙 확인
bool is_fifty_move_rule(const game_t *G) {
    return G->halfmove_clock >= 100;  // 50수 = 100 half-moves
}
