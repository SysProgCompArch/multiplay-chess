#ifndef CLIENT_UI_H
#define CLIENT_UI_H

#include <ncursesw/ncurses.h>

#include "colors.h"
#include "common.h"
#include "piece.h"

// UI 크기 상수들
#define SQUARE_W      7
#define SQUARE_H      3
#define BOARD_SIZE    8
#define BOARD_LABEL_X 4
#define BOARD_LABEL_Y 2
#define BOARD_BORDER  2
#define BOARD_WIDTH   (SQUARE_W * BOARD_SIZE + BOARD_LABEL_X * 2)
#define BOARD_HEIGHT  (SQUARE_H * BOARD_SIZE + BOARD_LABEL_Y * 2)

#define MAIN_MENU_HEIGHT 15
#define MAIN_MENU_WIDTH  50
#define DIALOG_HEIGHT    8
#define DIALOG_WIDTH     40

#define CHAT_HEIGHT  16
#define CHAT_WIDTH   35
#define INPUT_HEIGHT 3

#define MATCH_DIALOG_HEIGHT 10
#define MATCH_DIALOG_WIDTH  50

// ncurses 관련
void init_ncurses();
void init_colors();
void cleanup_ncurses();
void draw_border(WINDOW *win);

// 화면 그리기
void draw_main_screen();
void draw_matching_screen();
void draw_game_screen();

// 다이얼로그
bool get_username_dialog();
void show_error_dialog(const char *title, const char *message);

// 체스 보드 관련
void draw_chess_board(WINDOW *board_win);
void draw_chat_area(WINDOW *chat_win);
void draw_game_menu(WINDOW *menu_win);
char get_piece_char(piece_t *piece, int team);

// 입력 처리
void handle_mouse_input(MEVENT *event);
void handle_board_click(int board_x, int board_y);
bool coord_to_board_pos(int screen_x, int screen_y, int *board_x, int *board_y);

// 채팅 입력 처리
void enable_chat_input();
void disable_chat_input();
void handle_chat_input(int ch);
void draw_chat_input(WINDOW *input_win);

// 화면 업데이트
void update_match_timer();

#endif  // CLIENT_UI_H