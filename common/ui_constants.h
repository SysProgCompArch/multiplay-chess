#ifndef COMMON_UI_CONSTANTS_H
#define COMMON_UI_CONSTANTS_H

// ===================================================================
// 공통 UI 크기 상수들 (클라이언트 전용이지만 common에서 관리)
// ===================================================================

// 체스판 관련 상수
#define SQUARE_W      7
#define SQUARE_H      3
#define BOARD_SIZE    8
#define BOARD_LABEL_X 4
#define BOARD_LABEL_Y 2
#define BOARD_BORDER  2
#define BOARD_WIDTH   (SQUARE_W * BOARD_SIZE + BOARD_LABEL_X * 2)
#define BOARD_HEIGHT  (SQUARE_H * BOARD_SIZE + BOARD_LABEL_Y * 2)

// 윈도우 크기 상수
#define MAIN_MENU_HEIGHT 15
#define MAIN_MENU_WIDTH  50
#define DIALOG_HEIGHT    8
#define DIALOG_WIDTH     40

#define CHAT_HEIGHT  16
#define CHAT_WIDTH   35
#define INPUT_HEIGHT 3

#define MATCH_DIALOG_HEIGHT 10
#define MATCH_DIALOG_WIDTH  50

// 플레이어 정보창
#define PLAYER_INFO_HEIGHT 1

// 최소 터미널 크기
#define MIN_TERMINAL_WIDTH  100
#define MIN_TERMINAL_HEIGHT 30

// 채팅 관련 상수 (types.h에서 이동)
#define MAX_CHAT_LINES  12
#define MAX_CHAT_LENGTH 14

// 색상 쌍 정의 (colors.h에서 이동하여 통합)
#define COLOR_PAIR_BORDER          1
#define COLOR_PAIR_DIALOG_BORDER   2
#define COLOR_PAIR_SELECTED        3
#define COLOR_PAIR_BOARD_WHITE     4
#define COLOR_PAIR_BOARD_BLACK     5
#define COLOR_PAIR_SELECTED_SQUARE 6
#define COLOR_PAIR_CURSOR_SQUARE   7
#define COLOR_PAIR_WHITE_PLAYER    8
#define COLOR_PAIR_BLACK_PLAYER    9
#define COLOR_PAIR_CURRENT_TURN    10
#define COLOR_PAIR_POSSIBLE_MOVE   11
#define COLOR_PAIR_CAPTURE_MOVE    12
#define COLOR_PAIR_MY_PIECE        13
#define COLOR_PAIR_OPPONENT_PIECE  14

#endif  // COMMON_UI_CONSTANTS_H