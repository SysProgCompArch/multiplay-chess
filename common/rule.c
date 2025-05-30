// rule.c
#include "rule.h"
#include <stdbool.h>

// 보드 범위 체크
static inline bool in_board(int x, int y) {
    return x >= 0 && x < 8 && y >= 0 && y < 8;
}

// (x,y) 칸이 by_color 편에게 공격당하는지
static bool is_square_attacked(const game_t* G, int x, int y, int by_color)
{
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            const piecestate_t* ps = &G->board[i][j];
            if (ps->is_dead || ps->color != by_color) continue;

            switch (ps->piece->type) {
            case PAWN: {
                int dir = (by_color == WHITE ? +1 : -1);
                if (j + dir == y && (i - 1 == x || i + 1 == x))
                    return true;
                break;
            }
            case KNIGHT:
            case KING: {
                for (int k = 0; k < ps->piece->offset_len; k++) {
                    offset_t o = ps->piece->offset[k];
                    if (i + o.x == x && j + o.y == y)
                        return true;
                }
                break;
            }
            case BISHOP:
            case ROOK:
            case QUEEN: {
                for (int k = 0; k < ps->piece->offset_len; k++) {
                    offset_t o = ps->piece->offset[k];
                    int nx = i + o.x, ny = j + o.y;
                    while (in_board(nx, ny)) {
                        if (nx == x && ny == y) return true;
                        if (!G->board[nx][ny].is_dead) break;
                        nx += o.x; ny += o.y;
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
bool is_move_legal(const game_t* G, int sx, int sy, int dx, int dy)
{
    if (!in_board(sx, sy) || !in_board(dx, dy)) return false;
    const piecestate_t* src = &G->board[sx][sy];
    const piecestate_t* dst = &G->board[dx][dy];
    if (src->is_dead || src->color != G->side_to_move) return false;
    if (!dst->is_dead && dst->color == G->side_to_move) return false;

    int rx = dx - sx, ry = dy - sy;
    bool ok = false;

    switch (src->piece->type) {
    case PAWN: {
        int dir = (src->color == WHITE ? +1 : -1);
        // 한 칸 전진
        if (rx == 0 && ry == dir && dst->is_dead) ok = true;
        // 두 칸 전진
        else if (rx == 0 && ry == 2*dir && src->is_first_move
              && dst->is_dead
              && G->board[sx][sy+dir].is_dead) ok = true;
        // 대각선 캡처
        else if (abs(rx)==1 && ry==dir
              && (!dst->is_dead && dst->color!=src->color
                  || (dst->is_dead
                      && G->en_passant_x==dx && G->en_passant_y==dy)))
            ok = true;
        break;
    }
    case KNIGHT:
    case KING: {
        for (int k = 0; k < src->piece->offset_len; k++) {
            offset_t o = src->piece->offset[k];
            if (o.x==rx && o.y==ry) {
                // 캐슬링 처리
                if (src->piece->type==KING && abs(rx)==2 && ry==0) {
                    bool ks = rx > 0;
                    bool can = (src->color==WHITE)
                        ? (ks ? G->white_can_castle_kingside : G->white_can_castle_queenside)
                        : (ks ? G->black_can_castle_kingside : G->black_can_castle_queenside);
                    if (!can) return false;

                    int step = ks ? +1 : -1;
                    // 1) 중간 칸 비어있는지
                    for (int x = sx+step; x != dx; x += step)
                        if (!G->board[x][sy].is_dead) return false;
                    // 2) 지나가는 칸(출발칸, 중간칸, 도착칸) 모두 공격받지 않는지
                    for (int x = sx; x != dx+step; x += step)
                        if (is_square_attacked(G, x, sy, 1-src->color))
                            return false;
                }
                ok = true;
                break;
            }
        }
        break;
    }
    case BISHOP:
    case ROOK:
    case QUEEN: {
        for (int k = 0; k < src->piece->offset_len; k++) {
            offset_t o = src->piece->offset[k];
            int dd = o.x ? rx/o.x : ry/o.y;
            bool align = (o.x==0
                ? (rx==0 && ry/o.y>0)
                : (o.y==0
                    ? (ry==0 && rx/o.x>0)
                    : (rx%o.x==0 && ry%o.y==0 && dd==ry/o.y && dd>0)));
            if (!align) continue;
            // 중간 칸 확인
            bool blocked = false;
            for (int s = 1; s < dd; s++) {
                if (!G->board[sx+o.x*s][sy+o.y*s].is_dead) {
                    blocked = true;
                    break;
                }
            }
            if (!blocked) { ok = true; break; }
        }
        break;
    }
    }

    if (!ok) return false;

    // 자기 장군 방지
    game_t tmp = *G;
    apply_move(&tmp, sx, sy, dx, dy);
    if (is_in_check(&tmp, src->color)) return false;

    return true;
}

// 이동 적용
void apply_move(game_t* G, int sx, int sy, int dx, int dy)
{
    piecestate_t* src = &G->board[sx][sy];
    piecestate_t* dst = &G->board[dx][dy];

    // 50수 규칙 반영
    if (src->piece->type == PAWN || !dst->is_dead)
        G->halfmove_clock = 0;
    else
        G->halfmove_clock++;

    // 앙파상 캡처
    if (src->piece->type == PAWN
        && dx == G->en_passant_x && dy == G->en_passant_y) {
        piecestate_t* cap = &G->board[dx][sy];
        cap->is_dead = 1; cap->piece = NULL;
    }

    // 이동
    *dst = *src;
    dst->x = dx; dst->y = dy;
    src->is_dead = 1; src->piece = NULL;

    // 캐슬링 룩 이동
    if (dst->piece->type == KING && abs(dx-sx) == 2) {
        bool ks = dx > sx;
        int rook_from_x = ks ? 7 : 0;
        int rook_to_x   = ks ? dx-1 : dx+1;
        piecestate_t rook = G->board[rook_from_x][dy];
        rook.x = rook_to_x;
        G->board[rook_to_x][dy] = rook;
        G->board[rook_from_x][dy].is_dead = 1;
        G->board[rook_from_x][dy].piece = NULL;
    }

    // 캐슬링 권리 소멸
    if (dst->piece->type == KING) {
        if (dst->color == WHITE)
            G->white_can_castle_kingside = G->white_can_castle_queenside = false;
        else
            G->black_can_castle_kingside = G->black_can_castle_queenside = false;
    }
    if (dst->piece->type == ROOK && dst->is_first_move) {
        if (dst->color == WHITE) {
            if (sx==0 && sy==0) G->white_can_castle_queenside = false;
            if (sx==7 && sy==0) G->white_can_castle_kingside  = false;
        } else {
            if (sx==0 && sy==7) G->black_can_castle_queenside = false;
            if (sx==7 && sy==7) G->black_can_castle_kingside  = false;
        }
    }

    // 앙파상 타겟 갱신
    if (dst->piece->type == PAWN && abs(dy-sy) == 2) {
        G->en_passant_x = dx;
        G->en_passant_y = (sy + dy) / 2;
    } else {
        G->en_passant_x = G->en_passant_y = -1;
    }

    // 프로모션 (기본 퀸으로 자동 승격)
    if (dst->piece->type == PAWN && (dy == 7 || dy == 0)) {
        // extern으로 정의된 queen_piecestate를 가리키도록 하거나
        // 별도의 get_promotion_piece(color) 함수를 호출하세요.
        dst->piece = get_default_queen(dst->color);
        dst->is_promoted = 1;
    }

    dst->is_first_move = 0;
    G->side_to_move = 1 - G->side_to_move;
}
