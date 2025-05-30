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

// game_t �� ���� chessboard �迭�� ����
static void update_global_board(const game_t* G) {
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            chessboard[y][x] = G->board[x][y];
        }
    }
}

// ���� ���÷��� UI (move ��ȣ, ���� �ȳ�)
static void draw_replay_ui(int idx, int total) {
    werase(main_screen_win);
    box(main_screen_win, 0, 0);
    mvwprintw(main_screen_win, 1, 2, "Replay: %d / %d", idx, total);
    mvwprintw(main_screen_win, 3, 2, "<- Prev   Next ->");
    mvwprintw(main_screen_win, 5, 2, "Press 'q' to exit");
    wrefresh(main_screen_win);
}

// ���� ���÷��� ���
void replay_mode(const char* pgnfile) {
    // 1) PGN �ε�
    game_moves_t gm;
    if (!san_pgn_load(pgnfile, &gm)) {
        // �ε� ���� �� ������ ��ȯ
        return;
    }

    // 2) ���� ���� �ʱ�ȭ
    game_t G;
    init_startpos(&G);  // ������ġ ���� :contentReference[oaicite:4]{index=4}
    update_global_board(&G);
    draw_board();        // �ʱ��� �׸���
    draw_replay_ui(0, gm.count);

    // 3) Ű �Է����� ��/�� �̵�
    int idx = 0;
    int ch;
    // ȭ��ǥŰ �ν�
    keypad(stdscr, TRUE);
    while ((ch = getch()) != 'q') {
        if (ch == KEY_RIGHT && idx < gm.count) {
            // ���� �� ����
            move_t mv = gm.moves[idx];
            apply_move(&G, mv.sx, mv.sy, mv.dx, mv.dy);  // :contentReference[oaicite:5]{index=5}
            idx++;
        }
        else if (ch == KEY_LEFT && idx > 0) {
            // �ڷ� �� ��: ��ü ���� �� idx-1���� ������
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

        // ���� ������Ʈ & �ٽ� �׸���
        update_global_board(&G);
        draw_board();
        draw_replay_ui(idx, gm.count);
    }

    // 4) ����
    san_pgn_free(&gm);
}
