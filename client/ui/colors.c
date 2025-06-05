#include "colors.h"

#include "ui.h"

void init_colors() {
    // 색상 모드 활성화
    start_color();

    init_pair(COLOR_PAIR_BOARD_WHITE, CHESS_LIGHT_SQUARE, CHESS_DARK_SQUARE);
    init_pair(COLOR_PAIR_BOARD_BLACK, CHESS_DARK_SQUARE, CHESS_LIGHT_SQUARE);

    // 색상 설정
    if (COLORS >= 256) {
        init_pair(COLOR_PAIR_BORDER, CHESS_BORDER_COLOR, COLOR_BLACK);
        init_pair(COLOR_PAIR_SELECTED, EXT_COLOR_GOLD, COLOR_BLACK);
        init_pair(COLOR_PAIR_SELECTED_SQUARE, CHESS_SELECTED_SQUARE, COLOR_BLACK);
    } else {
        init_pair(COLOR_PAIR_BORDER, COLOR_CYAN, COLOR_BLACK);
        init_pair(COLOR_PAIR_SELECTED, COLOR_YELLOW, COLOR_BLACK);
        init_pair(COLOR_PAIR_SELECTED_SQUARE, COLOR_CYAN, COLOR_BLACK);
    }
}