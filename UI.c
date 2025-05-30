#define _XOPEN_SOURCE_EXTENDED
#include <locale.h>
#include <wchar.h>
#include <wctype.h>
#include <ncursesw/ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// 윈도우 크기 및 위치 정의
#define BOARD_HEIGHT   19
#define BOARD_WIDTH    33
#define BOARD_START_Y   1
#define BOARD_START_X   2

#define MAIN_HEIGHT    19
#define MAIN_WIDTH     26
#define MAIN_START_Y    1
#define MAIN_START_X   35

#define MAIN_SCREEN_HEIGHT   15
#define MAIN_SCREEN_WIDTH    24
#define MAIN_SCREEN_START_Y   4
#define MAIN_SCREEN_START_X  36

#define START_BUTTON_HEIGHT     3
#define START_BUTTON_WIDTH     10
#define START_BUTTON_START_Y    5
#define START_BUTTON_START_X   43

#define REPLAY_BUTTON_HEIGHT     3
#define REPLAY_BUTTON_WIDTH     10
#define REPLAY_BUTTON_START_Y    9
#define REPLAY_BUTTON_START_X   43

#define GAME_BUTTON_START_X   39
#define GAME_BUTTON_END_X     44
#define GAME_BUTTON_Y          2

#define MANUAL_BUTTON_START_X  49
#define MANUAL_BUTTON_END_X    56
#define MANUAL_BUTTON_Y         2

#define CHAT_HEIGHT    16
#define CHAT_WIDTH     27
#define CHAT_START_Y    1
#define CHAT_START_X   61

#define INPUT_HEIGHT    3
#define INPUT_WIDTH    27
#define INPUT_START_Y  17
#define INPUT_START_X  61

#define MAX_CHAT_LINES   12
#define MAX_CHAT_LENGTH  14

// 채팅, 명령어 입력 위치
#define CMD_LEFT 72
#define CMD_RIGHT 88
#define CMD_ROW 18

// 좌표 오프셋 타입
typedef struct offset { int x, y; } offset_t;
// 기물 종류(enum)
typedef enum piecetype { KING, QUEEN, ROOK, BISHOP, KNIGHT, PAWN } piecetype_t;
// 기물 기본 정보 구조체
typedef struct piece {
    char* name;
    piecetype_t type;
    offset_t* offset;
    int offset_len;
} piece_t;
// 기물 상태 구조체
typedef struct piecestate {
    piece_t* piece;
    int team, x, y;
    short is_dead, is_promoted;
} piecestate_t;

// 전역 윈도우 포인터
WINDOW *board_win, *main_win, *chat_win, *input_win, *main_screen_win, *start_button_win, *replay_button_win;
// 채팅 로그 버퍼
wchar_t chat_log[MAX_CHAT_LINES][MAX_CHAT_LENGTH + 1];
int chat_line_count = 0;
// 체스판 상태 저장 배열
piecestate_t chessboard[8][8];

// 함수 원형 선언
void init_colors();
piece_t* make_piece(const char* name, piecetype_t type);
void init_chess_board();
const wchar_t* get_unicode_piece(piecestate_t ps);
void init_windows();
void draw_manual();
void draw_game();
void draw_board();
void draw_main();
void draw_chat();
void draw_input(const wchar_t *buf, int len);
void free_pieces();
bool is_korean(wchar_t ch);
bool is_special_key(int res, wint_t wch) ;
void clear_korean_trace(int row, int col);

int main() {
    setlocale(LC_ALL, "");            // 로케일 설정
    initscr(); cbreak(); noecho(); refresh();     // ncurses 초기화
    keypad(stdscr, TRUE);               // 특수키 입력(on)
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL); //마우스 입력 on

    init_colors();                      // 색상 초기화
    init_chess_board();                 // 체스판 상태 초기화
    init_windows();                     // 윈도우 생성 및 테두리

    // 명령어/채팅 버퍼
    wchar_t cmd_buf[MAX_CHAT_LENGTH + 1] = L"";
    int cmd_len = 0;
    int current_width = 0;

    // 초기 화면 그리기
    draw_board(); draw_main(); draw_chat(); draw_input(cmd_buf, cmd_len);
    refresh(); 

    wint_t wch;
    MEVENT mev;
    while (1) {
        // 커서 위치 및 표시
        move(CMD_ROW, CMD_LEFT + current_width);
        curs_set(0);
        int res = get_wch(&wch);
        if (res == ERR) continue;

        // 마마우스 이벤트 처리
        if (res == KEY_CODE_YES && wch == KEY_MOUSE) {
            if (getmouse(&mev) == OK) {
                //왼쪽 버튼 클릭 감지
                if (mev.bstate & BUTTON1_CLICKED) {
                    int mx = mev.x, my = mev.y;
                    if(GAME_BUTTON_Y == my && GAME_BUTTON_START_X <= mx && GAME_BUTTON_END_X >= mx) {
                        draw_game();
                        continue;
                        
                    }
                    if(MANUAL_BUTTON_Y == my && MANUAL_BUTTON_START_X <= mx && MANUAL_BUTTON_END_X >= mx) {
                        draw_manual();
                        continue;
                    }
                }
            }
            continue;
        }

        // 한글 조합 잔상 제거 + 특수키키
        if (is_korean(wch) || is_special_key(res, wch)) {
            clear_korean_trace(CMD_ROW, CMD_LEFT + current_width);
            continue;
        }

        // Enter 입력 처리
        if (wch == L'\n') {
            if (cmd_len > 0) {
                // 명령어 처리
                if (cmd_buf[0] == L'/') {
                    if (wcscmp(cmd_buf, L"/exit") == 0) {
                        break;
                    } else if (wcscmp(cmd_buf, L"/clear") == 0) {
                        // 채팅 로그 초기화
                        for (int i = 0; i < MAX_CHAT_LINES; i++) chat_log[i][0] = L'\0';
                        chat_line_count = 0;
                    } else if (wcscmp(cmd_buf, L"/help") == 0) {
                        draw_manual();
                    } else if (wcscmp(cmd_buf, L"/game") == 0) {
                        draw_game();
                    } else if (wcscmp(cmd_buf, L"/replay") == 0) {
                        // TODO: PGN 리플레이 모드 처리
                    } else {
                        // Unknown command 처리
                        const wchar_t *msg = L"Unknown command.";
                        if (chat_line_count < MAX_CHAT_LINES) {
                            wcsncpy(chat_log[chat_line_count], msg, MAX_CHAT_LENGTH);
                            chat_log[chat_line_count][MAX_CHAT_LENGTH] = L'\0';
                            chat_line_count++;
                        } else {
                            for (int i = 1; i < MAX_CHAT_LINES; i++)
                                wcscpy(chat_log[i - 1], chat_log[i]);
                            wcsncpy(chat_log[MAX_CHAT_LINES - 1], msg, MAX_CHAT_LENGTH);
                            chat_log[MAX_CHAT_LINES - 1][MAX_CHAT_LENGTH] = L'\0';
                        }
                    }
                } else {
                    // 일반 채팅 입력 처리
                    if (chat_line_count < MAX_CHAT_LINES) {
                        wcsncpy(chat_log[chat_line_count], cmd_buf, MAX_CHAT_LENGTH);
                        chat_log[chat_line_count][MAX_CHAT_LENGTH] = L'\0';
                        chat_line_count++;
                    } else {
                        for (int i = 1; i < MAX_CHAT_LINES; i++)
                            wcscpy(chat_log[i - 1], chat_log[i]);
                        wcsncpy(chat_log[MAX_CHAT_LINES - 1], cmd_buf, MAX_CHAT_LENGTH);
                        chat_log[MAX_CHAT_LINES - 1][MAX_CHAT_LENGTH] = L'\0';
                    }
                }
            }
            // 버퍼 초기화
            cmd_len = 0; cmd_buf[0] = L'\0'; current_width = 0;
        } else if (wch == 127 || wch == KEY_BACKSPACE) {
            // Backspace 처리
            if (cmd_len > 0) {
                cmd_len--; cmd_buf[cmd_len] = L'\0'; current_width--;
            }
        } else {
            // 일반 문자 입력
            if (cmd_len < MAX_CHAT_LENGTH && iswprint(wch)) {
                cmd_buf[cmd_len++] = wch;
                cmd_buf[cmd_len] = L'\0'; current_width++;
            }
        }
        // 화면 갱신
        draw_board(); draw_main(); draw_chat(); draw_input(cmd_buf, cmd_len);
    }

    // 종료 처리
    delwin(board_win); delwin(main_win); delwin(chat_win); delwin(input_win);
    endwin();
    free_pieces();
    return 0;
}

// 색상 초기화
void init_colors() {
    start_color(); init_pair(1, COLOR_WHITE, COLOR_BLACK);
}

// piece 생성
piece_t* make_piece(const char* name, piecetype_t type) {
    piece_t* p = malloc(sizeof(piece_t));
    p->name = strdup(name);
    p->type = type; p->offset = NULL; p->offset_len = 0;
    return p;
}

// 체스판 초기화
void init_chess_board() {
    const piecetype_t back_row[8] = {ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK};
    for (int i = 0; i < 8; i++) {
        chessboard[0][i] = (piecestate_t){make_piece("P", back_row[i]), 0, i, 0, 0, 0};
        chessboard[1][i] = (piecestate_t){make_piece("P", PAWN), 0, i, 1, 0, 0};
        chessboard[6][i] = (piecestate_t){make_piece("P", PAWN), 1, i, 6, 0, 0};
        chessboard[7][i] = (piecestate_t){make_piece("P", back_row[i]), 1, i, 7, 0, 0};
    }
    for (int y = 2; y <= 5; y++) for (int x = 0; x < 8; x++)
        chessboard[y][x] = (piecestate_t){NULL, -1, x, y, 0, 0};
}

// Unicode 기호 반환
const wchar_t* get_unicode_piece(piecestate_t ps) {
    if (!ps.piece || ps.is_dead) return L" ";
    switch (ps.piece->type) {
        case KING:   return ps.team==0?L"♚":L"♔";
        case QUEEN:  return ps.team==0?L"♛":L"♕";
        case ROOK:   return ps.team==0?L"♜":L"♖";
        case BISHOP: return ps.team==0?L"♝":L"♗";
        case KNIGHT: return ps.team==0?L"♞":L"♘";
        case PAWN:   return ps.team==0?L"♟":L"♙";
    }
    return L" ";
}

// 윈도우 생성
void init_windows() {
    board_win = newwin(BOARD_HEIGHT, BOARD_WIDTH, BOARD_START_Y, BOARD_START_X);
    main_win  = newwin(MAIN_HEIGHT,  MAIN_WIDTH,  MAIN_START_Y,  MAIN_START_X);
    chat_win  = newwin(CHAT_HEIGHT,  CHAT_WIDTH,  CHAT_START_Y,  CHAT_START_X);
    input_win = newwin(INPUT_HEIGHT, INPUT_WIDTH, INPUT_START_Y, INPUT_START_X);
    main_screen_win = newwin(MAIN_SCREEN_HEIGHT, MAIN_SCREEN_WIDTH, MAIN_SCREEN_START_Y, MAIN_SCREEN_START_X);
    start_button_win = newwin(START_BUTTON_HEIGHT, START_BUTTON_WIDTH, START_BUTTON_START_Y, START_BUTTON_START_X);
    replay_button_win = newwin(REPLAY_BUTTON_HEIGHT, REPLAY_BUTTON_WIDTH, REPLAY_BUTTON_START_Y, REPLAY_BUTTON_START_X);

    box(board_win, 0, 0);
    box(main_win,  0, 0);
    box(chat_win,  0, 0);
    box(input_win, 0, 0);
}

// 체스판 그리기 (draw_board)
void draw_board() {
    werase(board_win);
    box(board_win, 0, 0);  // 테두리(box) 그리기

    // 파일(file) 표시: A ~ H
    mvwprintw(board_win, 0, 0, "┌─A─┬─B─┬─C─┬─D─┬─E─┬─F─┬─G─┬─H─┐");

    for (int r = 0; r < 8; r++) {
        int y = 1 + r * 2;

        

        // 칸(cell) 내부: 수직선(│) + 기물(piece)
        for (int c = 0; c < 8; c++) {
            int x = c * 4;
            mvwaddch(board_win, y, x, ACS_VLINE);
            mvwaddnwstr(board_win, y, x + 1,
                        get_unicode_piece(chessboard[r][c]), 1);
        }

        // 수평선(horizontal line) + 교차점(intersection) 그리기
        int hy = y + 1;
        mvwhline(board_win, hy, 1, ACS_HLINE, BOARD_WIDTH - 2);
        // 각 파일 경계 위치에 교차점(┼) 추가
        if(r == 7){
            for (int c = 1; c < 8; c++) {
                int ix = c * 4;
                mvwaddch(board_win, hy, ix, ACS_BTEE);
            }
        }
        else {
            for (int c = 1; c < 8; c++) {
                int ix = c * 4;
                mvwaddch(board_win, hy, ix, ACS_PLUS);
            }
        }
        mvwaddch(board_win, hy, 0, ACS_LTEE);
        mvwaddch(board_win, hy, 32, ACS_RTEE);
        // 랭크(rank) 표시: 8 ~
        mvwprintw(board_win, y, 0, "%d", 8 - r);
    } 
    wrefresh(board_win);
}

// 메인 그리기
void draw_main() {
    mvwprintw(main_win,1,4,"[game]"); mvwprintw(main_win,1,14,"[manual]");
    mvwprintw(main_win,2,0,"├────────────────────────┤");
    wrefresh(main_win);
}

// 채팅 그리기
void draw_chat() {
    werase(chat_win);
    box(chat_win, 0, 0);
    mvwprintw(chat_win,1,8,"[Live Chat]");
    mvwprintw(chat_win,2,0,"├─────────────────────────┤");
    int lines = chat_line_count > MAX_CHAT_LINES ? MAX_CHAT_LINES : chat_line_count;
    for (int i = 0; i < lines; i++) {
        mvwaddwstr(chat_win,
                    CHAT_HEIGHT - 1 - lines + i,
                    2,
                    chat_log[chat_line_count - lines + i]);
    }
    wrefresh(chat_win);
}

// 입력 그리기
void draw_input(const wchar_t* buf, int len) {
    werase(input_win);
    box(input_win, 0, 0);
    mvwprintw(input_win,1,1,"Command :");
    mvwaddwstr(input_win, 1, 11, buf);
    wrefresh(input_win);
}

//game 화면 그리기
void draw_game(){
    werase(main_screen_win);
    wrefresh(main_screen_win);
    werase(start_button_win);
    box(start_button_win, 0, 0);
    mvwprintw(start_button_win,1,2,"start");
    wrefresh(start_button_win);

    werase(replay_button_win);
    box(replay_button_win, 0, 0);
    mvwprintw(replay_button_win,1,2,"replay");
    wrefresh(replay_button_win);
}

//manual 화면 그리기
void draw_manual(){
    werase(main_screen_win);
    mvwprintw(main_screen_win,1,1,"/help : ~~");
    mvwprintw(main_screen_win,2,1,"/game : ~~");
    mvwprintw(main_screen_win,3,1,"/replay : ~~");
    mvwprintw(main_screen_win,4,1,"/clear : ~~");
    wrefresh(main_screen_win);
}

// 메모리 해제
void free_pieces() {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            if (chessboard[y][x].piece) {
                free(chessboard[y][x].piece->name);
                free(chessboard[y][x].piece);
            }
        }
    }
}

// 한글 IME 잔상 체크
bool is_korean(wchar_t ch) {
    return (ch >= 0xAC00 && ch <= 0xD7A3) ||
           (ch >= 0x3131 && ch <= 0x318E) ||
           (ch >= 0x1100 && ch <= 0x11FF);
}

//방향키 및 마우스
bool is_special_key(int res, wint_t wch) {
    return res == KEY_CODE_YES &&
           (wch == KEY_LEFT  || wch == KEY_RIGHT ||
            wch == KEY_UP    || wch == KEY_DOWN  ||
            wch == KEY_MOUSE);
}

void clear_korean_trace(int row, int col) {
    mvaddwstr(row, col, L" ");
    refresh();
    flushinp();
}
