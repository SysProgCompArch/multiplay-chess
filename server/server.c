// rule_test.c
#include <assert.h>
#include <stdio.h>
#include "rule.h"
#include "utils.h"

static void test_init_startpos() {
    game_t G;
    init_startpos(&G);

    // 1) 백 폰이 2련에 있어야
    for (int x = 0; x < 8; x++)
        assert(!G.board[x][1].is_dead && G.board[x][1].piece->type==PAWN);

    // 2) 흑 폰이 7련에 있어야
    for (int x = 0; x < 8; x++)
        assert(!G.board[x][6].is_dead && G.board[x][6].piece->type==PAWN);

    // 3) 왕·퀸 위치
    assert(G.board[4][0].piece->type==KING);
    assert(G.board[3][7].piece->type==QUEEN);

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
    bool ok = fen_parse("8/8/8/8/8/8/8/8 b - - 17 5", &G);
    assert(ok);
    // 빈 칸 확인
    for (int x=0;x<8;x++)for(int y=0;y<8;y++)
        assert(G.board[x][y].is_dead);
    assert(G.side_to_move==BLACK);
    assert(G.halfmove_clock==17);

    // 간단한 캐슬링 + 앙파상 테스트
    ok = fen_parse(
      "r3k2r/8/8/8/8/8/8/R3K2R w KQkq e3 0 1",
      &G
    );
    assert(ok);
    assert(G.board[0][0].piece->type==ROOK &&
           G.board[7][0].piece->type==ROOK);
    assert(G.white_can_castle_kingside &&
           G.white_can_castle_queenside);
    assert(G.en_passant_x==4 && G.en_passant_y==2);
}

int main() {
    test_init_startpos();
    test_fen_parser();
    printf("모든 테스트 통과 🎉\n");
    return 0;
}
