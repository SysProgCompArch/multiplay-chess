// replay.c
#define _XOPEN_SOURCE_EXTENDED
#include <locale.h>
#include <stdlib.h>
#include <ncursesw/ncurses.h>

#include "rule.h"    
#include "utils.h"   
#include "pgn.h"     

extern WINDOW* board_win, * main_screen_win;
extern piecestate_t chessboard[8][8];

// game_t → 전역 chessboard 배열로 복사
static void update_global_board(const game_t* G) {
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            chessboard[y][x] = G->board[x][y];
        }
    }
}

// 메인 리플레이 UI (move 번호, 조작 안내)
static void draw_replay_ui(int idx, int total) {
    werase(main_screen_win);
    box(main_screen_win, 0, 0);
    mvwprintw(main_screen_win, 1, 2, "Replay: %d / %d", idx, total);
    mvwprintw(main_screen_win, 3, 2, "<- Prev   Next ->");
    mvwprintw(main_screen_win, 5, 2, "Press 'q' to exit");
    wrefresh(main_screen_win);
}

// 실제 리플레이 모드
void replay_mode(const char* pgnfile) {
    // 1) PGN 로드
    game_moves_t gm;
    if (!san_pgn_load(pgnfile, &gm)) {
        // 로드 실패 시 간단히 반환
        return;
    }

    // 2) 게임 상태 초기화
    game_t G;
    init_startpos(&G);  // 시작위치 세팅 :contentReference[oaicite:4]{index=4}
    update_global_board(&G);
    draw_board();        // 초기판 그리기
    draw_replay_ui(0, gm.count);

    // 3) 키 입력으로 앞/뒤 이동
    int idx = 0;
    int ch;
    // 화살표키 인식
    keypad(stdscr, TRUE);
    while ((ch = getch()) != 'q') {
        if (ch == KEY_RIGHT && idx < gm.count) {
            // 다음 수 적용
            move_t mv = gm.moves[idx];
            apply_move(&G, mv.sx, mv.sy, mv.dx, mv.dy);  // :contentReference[oaicite:5]{index=5}
            idx++;
        }
        else if (ch == KEY_LEFT && idx > 0) {
            // 뒤로 한 수: 전체 리셋 후 idx-1까지 재적용
            init_startpos(&G);
            for (int i = 0; i < idx - 1; i++) {
                move_t mv = gm.moves[i];
                apply_move(&G, mv.sx, mv.sy, mv.dx, mv.dy);
            }
            idx--;
        }
        else {
            continue;
        }

        // 상태 업데이트 & 다시 그리기
        update_global_board(&G);
        draw_board();
        draw_replay_ui(idx, gm.count);
    }

    // 4) 정리
    san_pgn_free(&gm);
}
