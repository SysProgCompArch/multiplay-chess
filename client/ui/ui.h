#ifndef CLIENT_UI_H
#define CLIENT_UI_H

#include <ncursesw/ncurses.h>

#include "../game_state.h"
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
void handle_terminal_resize();

// 화면 그리기
void draw_main_screen();
void draw_matching_screen();
void draw_game_screen();

// 화면 전환 공통 함수
void draw_current_screen();

// 다이얼로그
bool get_username_dialog();
void show_dialog(const char *title, const char *message, const char *button_text);
void draw_dialog(const char *title, const char *message, const char *button_text);
void close_dialog();

// 체스 보드 관련
void draw_chess_board(WINDOW *board_win);
void draw_chat_area(WINDOW *chat_win);
void draw_game_menu(WINDOW *menu_win);
char get_piece_char(piece_t *piece, int team);

// 입력 처리
void handle_mouse_input(MEVENT *event);
void handle_board_click(int board_x, int board_y);
bool coord_to_board_pos(int screen_x, int screen_y, int *board_x, int *board_y);

// 키보드 조작 (방향키)
bool handle_keyboard_board_input(int ch);
void enable_cursor_mode();
void disable_cursor_mode();
bool is_cursor_mode();
void get_cursor_position(int *x, int *y);

// 체스 표기법 처리
bool handle_chess_notation(const char *notation);

// 채팅 입력 처리
void enable_chat_input();
void disable_chat_input();
void handle_chat_input(int ch);
void draw_chat_input(WINDOW *input_win);

// 화면 업데이트
void update_match_timer();

// 동적 크기 계산
void calculate_chat_dimensions(int terminal_cols, int *chat_width, int *input_width);

// 플레이어 정보 표시
void draw_player_info(WINDOW *player_win, const char *player_name, bool is_me, team_t team, bool is_current_turn, int time_remaining);

#endif  // CLIENT_UI_H