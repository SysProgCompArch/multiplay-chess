// rule_test.c
#include "rule.h"

#include <assert.h>
#include <stdio.h>

#include "utils.h"

static void test_init_startpos() {
    game_t G;
    init_startpos(&G);

    // 실제 보드 레이아웃 확인: board[y][x] = board[rank][file]
    // 1) 백 폰이 rank 1 (인덱스 1)에 있어야
    for (int x = 0; x < 8; x++)
        assert(!G.board[1][x].is_dead && G.board[1][x].piece->type == PIECE_PAWN);

    // 2) 흑 폰이 rank 6 (인덱스 6)에 있어야
    for (int x = 0; x < 8; x++)
        assert(!G.board[6][x].is_dead && G.board[6][x].piece->type == PIECE_PAWN);

    // 3) 왕·퀸 위치 - board[rank][file]
    assert(G.board[0][4].piece->type == PIECE_KING);   // 흰 킹 e1
    assert(G.board[7][3].piece->type == PIECE_QUEEN);  // 흑 퀸 d8

    // 4) 캐슬링 권리
    assert(G.white_can_castle_kingside &&
           G.white_can_castle_queenside);
    assert(G.black_can_castle_kingside &&
           G.black_can_castle_queenside);

    // 5) 앙파상·반수 초기값
    assert(G.en_passant_x == -1 && G.en_passant_y == -1);
    assert(G.halfmove_clock == 0);

    // 6) 흰 턴부터
    assert(G.side_to_move == WHITE);
}

static void test_fen_parser() {
    game_t G;
    // 테스트용 간단 포지션: 빈판, 흑 턴, 반수 17
    bool ok = fen_parse(&G, "8/8/8/8/8/8/8/8 b - - 17 5");
    assert(ok);
    // 빈 칸 확인 - board[y][x]
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++)
            assert(G.board[y][x].is_dead);
    assert(G.side_to_move == BLACK);
    assert(G.halfmove_clock == 17);

    // 간단한 캐슬링 + 앙파상 테스트
    ok = fen_parse(&G,
                   "r3k2r/8/8/8/8/8/8/R3K2R w KQkq e3 0 1");
    assert(ok);
    assert(G.board[0][0].piece->type == PIECE_ROOK &&  // a1 흰 룩
           G.board[0][7].piece->type == PIECE_ROOK);   // h1 흰 룩
    assert(G.white_can_castle_kingside &&
           G.white_can_castle_queenside);
    assert(G.en_passant_x == 4 && G.en_passant_y == 2);
}

static void test_pawn_move_f2_f4() {
    game_t G;
    init_startpos(&G);

    printf("Testing f2->f4 pawn move coordinates...\n");

    // f2 = file f(5), rank 2(1) → board[1][5]
    printf("f2 position: board[1][5]\n");
    piecestate_t *piece_f2 = &G.board[1][5];
    printf("  piece at f2: dead=%d, type=%d, team=%d, has_moved=%d\n",
           piece_f2->is_dead,
           piece_f2->piece ? piece_f2->piece->type : -1,
           piece_f2->team,
           piece_f2->has_moved);

    // f4 = file f(5), rank 4(3) → board[3][5]
    printf("f4 position: board[3][5]\n");
    piecestate_t *piece_f4 = &G.board[3][5];
    printf("  piece at f4: dead=%d, type=%d, team=%d\n",
           piece_f4->is_dead,
           piece_f4->piece ? piece_f4->piece->type : -1,
           piece_f4->team);

    // f3 중간 칸 확인 = file f(5), rank 3(2) → board[2][5]
    printf("f3 position (middle): board[2][5]\n");
    piecestate_t *piece_f3 = &G.board[2][5];
    printf("  piece at f3: dead=%d, type=%d, team=%d\n",
           piece_f3->is_dead,
           piece_f3->piece ? piece_f3->piece->type : -1,
           piece_f3->team);

    // 게임 상태 확인
    printf("\nGame state:\n");
    printf("  side_to_move: %d (0=WHITE, 1=BLACK)\n", G.side_to_move);

    // 폰 이동 조건 수동 확인
    printf("\nManual pawn move validation:\n");
    int sx = 5, sy = 1, dx = 5, dy = 3;  // f2->f4 (file, rank)
    int rx = dx - sx, ry = dy - sy;
    printf("  rx = %d, ry = %d\n", rx, ry);

    piecestate_t *src = &G.board[sy][sx];
    piecestate_t *dst = &G.board[dy][dx];

    printf("  src->team = %d, G.side_to_move = %d\n", src->team, G.side_to_move);
    printf("  src->team == G.side_to_move: %s\n", (src->team == G.side_to_move) ? "true" : "false");

    int dir = (src->team == TEAM_WHITE ? +1 : -1);
    printf("  dir = %d (WHITE=+1, BLACK=-1)\n", dir);
    printf("  2 * dir = %d\n", 2 * dir);

    printf("  Conditions for two-square pawn move:\n");
    printf("    rx == 0: %s\n", (rx == 0) ? "true" : "false");
    printf("    ry == 2 * dir: %s (%d == %d)\n", (ry == 2 * dir) ? "true" : "false", ry, 2 * dir);
    printf("    !src->has_moved: %s\n", (!src->has_moved) ? "true" : "false");
    printf("    dst->is_dead: %s\n", (dst->is_dead) ? "true" : "false");
    printf("    G.board[sy + dir][sx].is_dead: %s (board[%d][%d])\n",
           (G.board[sy + dir][sx].is_dead) ? "true" : "false", sy + dir, sx);

    // 자기 장군 방지 검사 디버깅
    printf("\nSelf-check prevention test:\n");
    game_t tmp = G;
    apply_move(&tmp, sx, sy, dx, dy);
    bool in_check_after = is_in_check(&tmp, src->team);
    printf("  After f2->f4, white king in check: %s\n", in_check_after ? "true" : "false");

    if (in_check_after) {
        printf("  This explains why the move is illegal!\n");
    }

    // f2->f4 이동 테스트 - is_move_legal(G, sx, sy, dx, dy)
    // sx=file(x), sy=rank(y) 이므로 (5, 1, 5, 3)
    printf("\nTesting f2->f4 move legality:\n");
    bool legal = is_move_legal(&G, 5, 1, 5, 3);
    printf("  is_move_legal(G, 5, 1, 5, 3) = %s\n", legal ? "true" : "false");

    if (!legal) {
        printf("  Move is illegal! This should be a valid pawn move.\n");
    }
}

int main() {
    test_init_startpos();
    test_fen_parser();
    test_pawn_move_f2_f4();
    printf("모든 테스트 통과 🎉\n");
    return 0;
}
