#define _XOPEN_SOURCE_EXTENDED

#include <locale.h>
#include <wchar.h>
#include <wctype.h>
#include <ncursesw/ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SCREEN_HEIGHT 20
#define SCREEN_WIDTH  100

#define CHAT_OUTLINE_TOP_ROW 0
#define CHAT_OUTLINE_TOP_COL 65
#define CHAT_OUTLINE_BOTTOM_ROW 19
#define CHAT_OUTLINE_BOTTOM_COL  92

#define MAIN_OUTLINE_TOP_ROW 0
#define MAIN_OUTLINE_TOP_COL 36
#define MAIN_OUTLINE_BOTTOM_ROW 19
#define MAIN_OUTLINE_BOTTOM_COL  63

#define MAIN_SCREEN_TOP_ROW 3
#define MAIN_SCREEN_TOP_COL 37
#define MAIN_SCREEN_BOTTOM_ROW 18
#define MAIN_SCREEN_BOTTOM_COL  62

#define BOARD_OUTLINE_TOP_ROW 1
#define BOARD_OUTLINE_TOP_COL 2
#define BOARD_OUTLINE_BOTTOM_ROW (BOARD_OUTLINE_TOP_ROW + 16)
#define BOARD_OUTLINE_BOTTOM_COL (BOARD_OUTLINE_TOP_COL + 32)

#define START_BUTTON_OUTLINE_TOP_ROW 5
#define START_BUTTON_OUTLINE_TOP_COL 42
#define START_BUTTON_OUTLINE_BOTTOM_ROW (START_BUTTON_OUTLINE_TOP_ROW + 2)
#define START_BUTTON_OUTLINE_BOTTOM_COL (START_BUTTON_OUTLINE_TOP_COL + 15)

#define REPLAY_BUTTON_OUTLINE_TOP_ROW 9
#define REPLAY_BUTTON_OUTLINE_TOP_COL 42
#define REPLAY_BUTTON_OUTLINE_BOTTOM_ROW (REPLAY_BUTTON_OUTLINE_TOP_ROW + 2)
#define REPLAY_BUTTON_OUTLINE_BOTTOM_COL (REPLAY_BUTTON_OUTLINE_TOP_COL + 15)

#define GAME_BUTTON_LEFT 37
#define GAME_BUTTON_RIGHT (GAME_BUTTON_LEFT + 11)
#define GAME_BUTTON_ROW 1

#define MANUAL_BUTTON_LEFT 50
#define MANUAL_BUTTON_RIGHT (MANUAL_BUTTON_LEFT + 12)
#define MANUAL_BUTTON_ROW 1

#define CHAT_BUTTON_LEFT 66
#define CHAT_BUTTON_RIGHT (CHAT_BUTTON_LEFT + 25)
#define CHAT_BUTTON_ROW 1

#define CMD_LEFT 76
#define CMD_RIGHT 91
#define CMD_ROW 18

#define MAX_CHAT_LINES 15
#define MAX_CHAT_LENGTH 50

wchar_t chat_log[MAX_CHAT_LINES][MAX_CHAT_LENGTH + 1];
int chat_line_count = 0;


typedef struct offset {
    int x, y;
} offset_t;

typedef enum piecetype {
    KING, QUEEN, ROOK, BISHOP, KNIGHT, PAWN
} piecetype_t;

typedef struct piece {
    char* name;
    piecetype_t type;
    offset_t* offset;
    int offset_len;
} piece_t;

typedef struct piecestate {
    piece_t* piece;
    int team;
    int x, y;
    short is_dead, is_promoted;
} piecestate_t;

piecestate_t chessboard[8][8];

typedef struct screenbuffer {
    wchar_t c;
    short color_pair;
} screenbuffer_t;

screenbuffer_t screen[SCREEN_HEIGHT][SCREEN_WIDTH];

void init_colors() {
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
}

const wchar_t* get_unicode_piece(piecestate_t ps) {
    if (!ps.piece || ps.is_dead) return L" ";
    switch (ps.piece->type) {
        case KING:   return ps.team == 0 ? L"♚" : L"♔";
        case QUEEN:  return ps.team == 0 ? L"♛" : L"♕";
        case ROOK:   return ps.team == 0 ? L"♜" : L"♖";
        case BISHOP: return ps.team == 0 ? L"♝" : L"♗";
        case KNIGHT: return ps.team == 0 ? L"♞" : L"♘";
        case PAWN:   return ps.team == 0 ? L"♟" : L"♙";
    }
    return L" ";
}

piece_t* make_piece(const char* name, piecetype_t type) {
    piece_t* p = malloc(sizeof(piece_t));
    p->name = strdup(name);
    p->type = type;
    p->offset = NULL;
    p->offset_len = 0;
    return p;
}


const wchar_t* sample_pgn[] = {
    L"[Event \"Friendly Match\"]",
    L"[Site \"Daegu\"]",
    L"[Date \"2025.05.18\"]",
    L"[Round \"1\"]",
    L"[White \"Player1\"]",
    L"[Black \"Player2\"]",
    L"[Result \"1-0\"]",
    L"",
    L"1. e4 e5",
    L"2. Nf3 Nc6",
    L"3. Bb5 a6",
    L"4. Ba4 Nf6",
    L"5. O-O Be7",
    L"6. Re1 b5",
    L"7. Bb3 d6",
    L"8. c3 O-O",
    L"9. h3 Nb8",
    L"10. d4 Nbd7",
    L"",
    L"1-0"
};
const int pgn_line_count = sizeof(sample_pgn) / sizeof(sample_pgn[0]);


void draw_outline(int top_row, int top_col, int bottom_row, int bottom_col) {
    for (int x = top_col + 1; x < bottom_col; x++) {
        screen[top_row][x] = (screenbuffer_t){L'-', 1};
        screen[bottom_row][x] = (screenbuffer_t){L'-', 1};
    }
    for (int y = top_row + 1; y < bottom_row; y++) {
        screen[y][top_col] = (screenbuffer_t){L'|', 1};
        screen[y][bottom_col] = (screenbuffer_t){L'|', 1};
    }
    screen[top_row][top_col] = (screenbuffer_t){L'+', 1};
    screen[top_row][bottom_col] = (screenbuffer_t){L'+', 1};
    screen[bottom_row][top_col] = (screenbuffer_t){L'+', 1};
    screen[bottom_row][bottom_col] = (screenbuffer_t){L'+', 1};
}

void init_chess_board() {
    const piecetype_t back_row[8] = {ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK};
    for (int i = 0; i < 8; i++) {
        chessboard[0][i] = (piecestate_t){make_piece("P", back_row[i]), 0, i, 0, 0, 0};
        chessboard[1][i] = (piecestate_t){make_piece("P", PAWN), 0, i, 1, 0, 0};
        chessboard[6][i] = (piecestate_t){make_piece("P", PAWN), 1, i, 6, 0, 0};
        chessboard[7][i] = (piecestate_t){make_piece("P", back_row[i]), 1, i, 7, 0, 0};
    }
    for (int y = 2; y <= 5; y++)
        for (int x = 0; x < 8; x++)
            chessboard[y][x] = (piecestate_t){NULL, -1, x, y, 0, 0};
}

void clear_screen_buffer() {
    for (int y = 0; y < SCREEN_HEIGHT; y++)
        for (int x = 0; x < SCREEN_WIDTH; x++)
            screen[y][x] = (screenbuffer_t){L' ', 1};
}

void clear_main_screen() {
    for (int y = MAIN_SCREEN_TOP_ROW; y <= MAIN_SCREEN_BOTTOM_ROW; y++)
            for (int x = MAIN_SCREEN_TOP_COL; x <= MAIN_SCREEN_BOTTOM_COL; x++)
                screen[y][x] = (screenbuffer_t){L' ', 1};
} 

void draw_start_screen() {
    const wchar_t* start = L"start";
    const wchar_t* replay = L"replay";

    draw_outline(START_BUTTON_OUTLINE_TOP_ROW, START_BUTTON_OUTLINE_TOP_COL,
                 START_BUTTON_OUTLINE_BOTTOM_ROW, START_BUTTON_OUTLINE_BOTTOM_COL);

    draw_outline(REPLAY_BUTTON_OUTLINE_TOP_ROW, REPLAY_BUTTON_OUTLINE_TOP_COL,
                 REPLAY_BUTTON_OUTLINE_BOTTOM_ROW, REPLAY_BUTTON_OUTLINE_BOTTOM_COL);  
                 

    for(int i = 0; i < wcslen(start); i++){
        screen[START_BUTTON_OUTLINE_TOP_ROW + 1][START_BUTTON_OUTLINE_TOP_COL + 5 + i] = (screenbuffer_t){start[i], 1};
    }

    for(int i = 0; i < wcslen(replay); i++){
        screen[REPLAY_BUTTON_OUTLINE_TOP_ROW + 1][REPLAY_BUTTON_OUTLINE_TOP_COL + 5 + i] = (screenbuffer_t){replay[i], 1};
    }
}

void draw_manual_screen() {
    const wchar_t* manual[] = {
        L"/help",
        L"/exit",
        L"/clear"
    };
    int manual_len = sizeof(manual) / sizeof(manual[0]); 
    for (int y = 0; y < manual_len; y++)
            for (int x = 0; x < wcslen(manual[y]); x++)
                screen[MAIN_SCREEN_TOP_ROW + 1 + y][MAIN_SCREEN_TOP_COL + 1 + x] = (screenbuffer_t){manual[y][x], 1};
}

void draw_pgn_page(int page) {
    clear_main_screen();
    const int lines_per_page = MAIN_SCREEN_BOTTOM_ROW - MAIN_SCREEN_TOP_ROW - 1;
    int start = page * lines_per_page;
    for (int i = 0; i < lines_per_page; i++) {
        if (start + i >= pgn_line_count) break;
        for (int j = 0; j < wcslen(sample_pgn[start + i]); j++) {
            if (MAIN_SCREEN_TOP_COL + 1 + j > MAIN_SCREEN_BOTTOM_COL) break;
            screen[MAIN_SCREEN_TOP_ROW + 1 + i][MAIN_SCREEN_TOP_COL + 1 + j] =
                (screenbuffer_t){sample_pgn[start + i][j], 1};
        }
    }
}


void init_screen_buffer() {
    const wchar_t* files = L"A   B   C   D   E   F   G   H";
    const wchar_t* chess_line = L"+---+---+---+---+---+---+---+---+";
    const wchar_t* manual = L"manual";
    const wchar_t* game = L"game";
    const wchar_t* live_chat = L"live chat";
    const wchar_t* cmd = L"command : ";

    for (int i = 0; i < wcslen(files); i++)
        screen[0][4 + i] = (screenbuffer_t){files[i], 1};

    for (int i = 0; i <= 16; i += 2)
        for (int j = 0; j < wcslen(chess_line); j++)
            screen[BOARD_OUTLINE_TOP_ROW + i][BOARD_OUTLINE_TOP_COL + j] = (screenbuffer_t){chess_line[j], 1};

    for (int row = 0; row < 8; row++) {
        int y = BOARD_OUTLINE_TOP_ROW + 1 + row * 2;
        screen[y][0] = (screenbuffer_t){L'8' - row, 1};
        for (int col = 0; col < 8; col++) {
            int x = BOARD_OUTLINE_TOP_COL + col * 4;
            screen[y][x] = (screenbuffer_t){L'|', 1};
            const wchar_t* uni = get_unicode_piece(chessboard[row][col]);
            screen[y][x + 2] = (screenbuffer_t){uni[0], 1};
        }
        screen[y][BOARD_OUTLINE_BOTTOM_COL] = (screenbuffer_t){L'|', 1};
    }

    screen[BOARD_OUTLINE_BOTTOM_ROW + 1][BOARD_OUTLINE_TOP_COL] = (screenbuffer_t){L'|', 1};
    screen[BOARD_OUTLINE_BOTTOM_ROW + 1][BOARD_OUTLINE_BOTTOM_COL] = (screenbuffer_t){L'|', 1};

    for(int i = BOARD_OUTLINE_TOP_COL + 1; i < BOARD_OUTLINE_BOTTOM_COL; i++){
        screen[BOARD_OUTLINE_BOTTOM_ROW + 2][i] = (screenbuffer_t){L'-', 1};
    }

    screen[BOARD_OUTLINE_BOTTOM_ROW + 2][BOARD_OUTLINE_TOP_COL] = (screenbuffer_t){L'+', 1};
    screen[BOARD_OUTLINE_BOTTOM_ROW + 2][BOARD_OUTLINE_BOTTOM_COL] = (screenbuffer_t){L'+', 1};

    draw_outline(MAIN_OUTLINE_TOP_ROW, MAIN_OUTLINE_TOP_COL,
                 MAIN_OUTLINE_BOTTOM_ROW, MAIN_OUTLINE_BOTTOM_COL);

    for(int i = MAIN_OUTLINE_TOP_COL + 1; i < MAIN_OUTLINE_BOTTOM_COL; i++){
        screen[MAIN_OUTLINE_TOP_ROW + 2][i] = (screenbuffer_t){L'-', 1};
    }

    screen[MAIN_OUTLINE_TOP_ROW + 2][MAIN_OUTLINE_TOP_COL] = (screenbuffer_t){L'+', 1};
    screen[MAIN_OUTLINE_TOP_ROW + 2][MAIN_OUTLINE_BOTTOM_COL] = (screenbuffer_t){L'+', 1};
    screen[MAIN_OUTLINE_TOP_ROW + 1][(MAIN_OUTLINE_TOP_COL+MAIN_OUTLINE_BOTTOM_COL)/2] = (screenbuffer_t){L'|', 1};

    draw_outline(CHAT_OUTLINE_TOP_ROW, CHAT_OUTLINE_TOP_COL,
                 CHAT_OUTLINE_BOTTOM_ROW, CHAT_OUTLINE_BOTTOM_COL);

    for(int i = CHAT_OUTLINE_TOP_COL + 1; i < CHAT_OUTLINE_BOTTOM_COL; i++){
        screen[CHAT_OUTLINE_TOP_ROW + 2][i] = (screenbuffer_t){L'-', 1};
    }

    screen[CHAT_OUTLINE_TOP_ROW + 2][CHAT_OUTLINE_TOP_COL] = (screenbuffer_t){L'+', 1};
    screen[CHAT_OUTLINE_TOP_ROW + 2][CHAT_OUTLINE_BOTTOM_COL] = (screenbuffer_t){L'+', 1};

    for(int i = CHAT_OUTLINE_TOP_COL + 1; i < CHAT_OUTLINE_BOTTOM_COL; i++){
        screen[CHAT_OUTLINE_BOTTOM_ROW - 2][i] = (screenbuffer_t){L'-', 1};
    }

    screen[CHAT_OUTLINE_BOTTOM_ROW - 2][CHAT_OUTLINE_TOP_COL] = (screenbuffer_t){L'+', 1};
    screen[CHAT_OUTLINE_BOTTOM_ROW - 2][CHAT_OUTLINE_BOTTOM_COL] = (screenbuffer_t){L'+', 1};



    

    for(int i = 0; i < wcslen(game); i++){
        screen[GAME_BUTTON_ROW][GAME_BUTTON_LEFT + 4 + i] = (screenbuffer_t){game[i], 1};
    }
    
    for(int i = 0; i < wcslen(manual); i++){
        screen[MANUAL_BUTTON_ROW][MANUAL_BUTTON_LEFT + 4 + i] = (screenbuffer_t){manual[i], 1};
    }

    for(int i = 0; i < wcslen(live_chat); i++){
        screen[CHAT_BUTTON_ROW][CHAT_BUTTON_LEFT + 9 + i] = (screenbuffer_t){live_chat[i], 1};
    }

    for(int i = 0; i < wcslen(cmd); i++){
        screen[CMD_ROW][CMD_LEFT - 10 + i] = (screenbuffer_t){cmd[i], 1};
    }

    draw_start_screen();
}

void draw_chat_log() {
    int start_row = CHAT_OUTLINE_TOP_ROW + 3;
    int end_row = CHAT_OUTLINE_BOTTOM_ROW - 3;

    for (int y = start_row; y <= end_row; y++) {
        for (int x = CHAT_OUTLINE_TOP_COL + 1; x < CHAT_OUTLINE_BOTTOM_COL; x++) {
            screen[y][x] = (screenbuffer_t){L' ', 1};
        }
    }

    int row = end_row;
    int printed = 0;
    for (int i = chat_line_count - 1; i >= 0 && printed < (end_row - start_row + 1); i--, row--, printed++) {
        int col = CHAT_OUTLINE_TOP_COL + 2;
        for (int j = 0; j < wcslen(chat_log[i]) && col < CHAT_OUTLINE_BOTTOM_COL; j++, col++) {
            screen[row][col] = (screenbuffer_t){chat_log[i][j], 1};
        }
    }
}


void print_screen_buffer() {
    for (int y = 0; y < SCREEN_HEIGHT; y++)
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            attron(COLOR_PAIR(screen[y][x].color_pair));
            mvaddnwstr(y, x, &screen[y][x].c, 1);
            attroff(COLOR_PAIR(screen[y][x].color_pair));
        }
}

void free_pieces() {
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++)
            if (chessboard[y][x].piece) {
                free(chessboard[y][x].piece->name);
                free(chessboard[y][x].piece);
            }
}

bool is_korean(wchar_t ch) {
    return  (ch >= 0xAC00 && ch <= 0xD7A3) ||  // 완성형
            (ch >= 0x3131 && ch <= 0x318E) ||  // 자음 (ㄱ~ㆎ)
            (ch >= 0x1100 && ch <= 0x11FF);    // 모음 자모
}

void clear_korean_trace(int row, int col) {
    mvaddwstr(row, col, L" ");  // 한글 조각 덮어쓰기
    refresh();                  // 즉시 반영
    flushinp();                 // IME 조합 잔상도 제거
}

int main() {
    setlocale(LC_ALL, "");
    initscr();
    noecho();
    cbreak();

    init_colors();
    clear_screen_buffer();
    init_chess_board();
    init_screen_buffer();
    print_screen_buffer();
    refresh();

    wchar_t cmd_buf[CMD_RIGHT - CMD_LEFT + 2];
    int cmd_len = 0;
    int max_width = CMD_RIGHT - CMD_LEFT + 1;
    int current_width = 0;

    while (1) {
        wchar_t ch;
        move(CMD_ROW, CMD_LEFT + current_width);
        curs_set(0); 
        int result = get_wch(&ch);
        if (result == ERR) continue;
        if (is_korean(ch)) {
            clear_korean_trace(CMD_ROW, CMD_LEFT + current_width);
            continue;
        }
        if (ch == L'\n') {
            if (cmd_len > 0) {
                // 명령어 처리
                if (cmd_buf[0] == L'/') {
                    if (wcscmp(cmd_buf, L"/exit") == 0) {
                        break;  // 프로그램 종료
                    }else if (wcscmp(cmd_buf, L"/clear") == 0) {
                        // ✅ /clear 명령어 처리
                        for (int i = 0; i < MAX_CHAT_LINES; i++)
                            chat_log[i][0] = L'\0';
                        chat_line_count = 0;
                    }else if (wcscmp(cmd_buf, L"/help") == 0) {
                        clear_main_screen();
                        draw_manual_screen();
                        print_screen_buffer(); 
                    }else if (wcscmp(cmd_buf, L"/game") == 0) {
                        clear_main_screen();
                        draw_start_screen();
                        print_screen_buffer(); 
                    }else if (wcscmp(cmd_buf, L"/replay") == 0) {
                        int page = 0;
                        int max_page = (pgn_line_count - 1) / (MAIN_SCREEN_BOTTOM_ROW - MAIN_SCREEN_TOP_ROW - 1);

                        while (1) {
                            draw_pgn_page(page);
                            print_screen_buffer();
                            int ch = getch();
                            if (ch == 'q') {
                                clear_main_screen();
                                draw_start_screen();
                                print_screen_buffer(); 
                                break;
                            }
                            else if (ch == 'n' && page < max_page) page++;
                            else if (ch == 'p' && page > 0) page--;
                        }
                    }else {
                        // ✅ 영어로만 "Unknown command." 출력
                        const wchar_t* unknown_msg = L"Unknown command.";

                        if (chat_line_count < MAX_CHAT_LINES) {
                            wcsncpy(chat_log[chat_line_count], unknown_msg, MAX_CHAT_LENGTH);
                            chat_log[chat_line_count][MAX_CHAT_LENGTH] = L'\0';
                            chat_line_count++;
                        } else {
                            for (int i = 1; i < MAX_CHAT_LINES; i++)
                                wcscpy(chat_log[i - 1], chat_log[i]);
                            wcsncpy(chat_log[MAX_CHAT_LINES - 1], unknown_msg, MAX_CHAT_LENGTH);
                            chat_log[MAX_CHAT_LINES - 1][MAX_CHAT_LENGTH] = L'\0';
                        }
                    }
                    // 나중에 다른 명령어 처리도 여기에 추가 가능
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

            cmd_len = 0;
            cmd_buf[0] = L'\0';
            current_width = 0;
        }

       if (ch == 127 || ch == KEY_BACKSPACE) {
            if (cmd_len > 0) {
                cmd_len--;
                cmd_buf[cmd_len] = L'\0';
                current_width--;
            }
        } else {
            if (cmd_len < max_width && iswprint(ch)) {
                cmd_buf[cmd_len++] = ch;
                cmd_buf[cmd_len] = L'\0';
                current_width++;
            }
        }
        for (int i = CMD_LEFT; i <= CMD_RIGHT; i++)
        screen[CMD_ROW][i] = (screenbuffer_t){L' ', 1};

        // 입력 문자열 출력
        int draw_col = CMD_LEFT;
        for (int i = 0; i < cmd_len && draw_col <= CMD_RIGHT; i++, draw_col++) {
            screen[CMD_ROW][draw_col] = (screenbuffer_t){cmd_buf[i], 1};
        }

        draw_chat_log();
        print_screen_buffer();
        refresh();
    }

    endwin();
    free_pieces();
    return 0;
}