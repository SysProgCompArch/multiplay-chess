#define _XOPEN_SOURCE_EXTENDED

#include <locale.h>
#include <wchar.h>
#include <ncursesw/ncurses.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_HEIGHT 20
#define SCREEN_WIDTH  60
#define SCREEN_MENU_COL 35

// ▶ 메인화면 범위
#define MENU_TOP_ROW      3
#define MENU_TOP_COL      (SCREEN_MENU_COL + 1)
#define MENU_BOTTOM_ROW   19
#define MENU_BOTTOM_COL   (SCREEN_MENU_COL + 22)

// 버튼/텍스트 위치 상수
#define MENU_BUTTON_ROW          1
#define MENU_GAME_COL_FRONT   (SCREEN_MENU_COL + 1)
#define MENU_GAME_COL_REAR    (SCREEN_MENU_COL + 10)
#define MENU_MANUAL_COL_FRONT (SCREEN_MENU_COL + 14)
#define MENU_MANUAL_COL_REAR  (SCREEN_MENU_COL + 21)

#define REPLAY_BOX_TOP_ROW    4
#define REPLAY_TEXT_ROW       5
#define REPLAY_TEXT_COL       (SCREEN_MENU_COL + 9)

#define START_BOX_TOP_ROW     8
#define START_TEXT_ROW        9
#define START_TEXT_COL        (SCREEN_MENU_COL + 9) 

#define MANUAL_START_ROW      4
#define MANUAL_START_COL      (SCREEN_MENU_COL + 2)

// offset 구조체
typedef struct offset {
    int x;
    int y;
} offset_t;

// 기물 타입
typedef enum piecetype {
    KING, QUEEN, ROOK, BISHOP, KNIGHT, PAWN
} piecetype_t;

// 기물 정보 구조체
typedef struct piece {
    char* name;
    piecetype_t type;
    offset_t* offset;
    int offset_len;
} piece_t;

// 기물 상태 구조체
typedef struct piecestate {
    piece_t* piece;
    int team;
    int x, y;
    short is_dead;
    short is_promoted;
} piecestate_t;

// 체스판
piecestate_t chessboard[8][8];

// 화면 버퍼 구조체
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
    if (ps.piece == NULL || ps.is_dead) return L" ";
    switch (ps.piece->type) {
        case KING:   return (ps.team == 0) ? L"♚" : L"♔";
        case QUEEN:  return (ps.team == 0) ? L"♛" : L"♕";
        case ROOK:   return (ps.team == 0) ? L"♜" : L"♖";
        case BISHOP: return (ps.team == 0) ? L"♝" : L"♗";
        case KNIGHT: return (ps.team == 0) ? L"♞" : L"♘";
        case PAWN:   return (ps.team == 0) ? L"♟" : L"♙";
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

void init_chess_board() {
    chessboard[0][0] = (piecestate_t){make_piece("Rook", ROOK), 0, 0, 0, 0, 0};
    chessboard[0][1] = (piecestate_t){make_piece("Knight", KNIGHT), 0, 1, 0, 0, 0};
    chessboard[0][2] = (piecestate_t){make_piece("Bishop", BISHOP), 0, 2, 0, 0, 0};
    chessboard[0][3] = (piecestate_t){make_piece("Queen", QUEEN), 0, 3, 0, 0, 0};
    chessboard[0][4] = (piecestate_t){make_piece("King", KING), 0, 4, 0, 0, 0};
    chessboard[0][5] = (piecestate_t){make_piece("Bishop", BISHOP), 0, 5, 0, 0, 0};
    chessboard[0][6] = (piecestate_t){make_piece("Knight", KNIGHT), 0, 6, 0, 0, 0};
    chessboard[0][7] = (piecestate_t){make_piece("Rook", ROOK), 0, 7, 0, 0, 0};
    for (int i = 0; i < 8; i++) {
        chessboard[1][i] = (piecestate_t){make_piece("Pawn", PAWN), 0, i, 1, 0, 0};
        chessboard[6][i] = (piecestate_t){make_piece("Pawn", PAWN), 1, i, 6, 0, 0};
    }
    chessboard[7][0] = (piecestate_t){make_piece("Rook", ROOK), 1, 0, 7, 0, 0};
    chessboard[7][1] = (piecestate_t){make_piece("Knight", KNIGHT), 1, 1, 7, 0, 0};
    chessboard[7][2] = (piecestate_t){make_piece("Bishop", BISHOP), 1, 2, 7, 0, 0};
    chessboard[7][3] = (piecestate_t){make_piece("Queen", QUEEN), 1, 3, 7, 0, 0};
    chessboard[7][4] = (piecestate_t){make_piece("King", KING), 1, 4, 7, 0, 0};
    chessboard[7][5] = (piecestate_t){make_piece("Bishop", BISHOP), 1, 5, 7, 0, 0};
    chessboard[7][6] = (piecestate_t){make_piece("Knight", KNIGHT), 1, 6, 7, 0, 0};
    chessboard[7][7] = (piecestate_t){make_piece("Rook", ROOK), 1, 7, 7, 0, 0};

    for (int y = 2; y <= 5; y++)
        for (int x = 0; x < 8; x++)
            chessboard[y][x] = (piecestate_t){NULL, -1, x, y, 0, 0};
}

void clear_screen_buffer() {
    for (int y = 0; y < SCREEN_HEIGHT; y++)
        for (int x = 0; x < SCREEN_WIDTH; x++)
            screen[y][x] = (screenbuffer_t){L' ', 1};
}

void clear_main_screen_buffer() {
    for (int y = MENU_TOP_ROW; y < MENU_BOTTOM_ROW; y++)
        for (int x = MENU_TOP_COL; x < MENU_BOTTOM_COL; x++)
            screen[y][x] = (screenbuffer_t){L' ', 1};
}

void main_manual_view() {
    const wchar_t* manual[] = {
        L"hexteacher",
    };
    int manual_lines = sizeof(manual) / sizeof(manual[0]);

    for (int i = 0; i < manual_lines; i++) {
        for (int j = 0; j < wcslen(manual[i]); j++) {
            screen[MANUAL_START_ROW + i][MANUAL_START_COL + j] = (screenbuffer_t){manual[i][j], 1};
        }
    }
}

void main_game_view() {
    const wchar_t* line = L"+---------------+";
    const wchar_t* game_start = L"start";
    const wchar_t* game_replay = L"replay";

    for(int i = 0; i <= 2; i++) {
        int row = REPLAY_BOX_TOP_ROW + i;
        if(i == 1){
            screen[row][SCREEN_MENU_COL + 3] = (screenbuffer_t){L'|', 1};
            screen[row][SCREEN_MENU_COL + 2 + wcslen(line)] = (screenbuffer_t){L'|', 1};
        } else {
            for (int j = 0; j < wcslen(line); j++) {
                screen[row][j + SCREEN_MENU_COL + 3] = (screenbuffer_t){line[j], 1};
            }
        }
    }

    for(int i = 0; i <= 2; i++) {
        int row = START_BOX_TOP_ROW + i;
        if(i == 1){
            screen[row][SCREEN_MENU_COL + 3] = (screenbuffer_t){L'|', 1};
            screen[row][SCREEN_MENU_COL + 2 + wcslen(line)] = (screenbuffer_t){L'|', 1};
        } else {
            for (int j = 0; j < wcslen(line); j++) {
                screen[row][j + SCREEN_MENU_COL + 3] = (screenbuffer_t){line[j], 1};
            }
        }
    }

    for(int i = 0; i < wcslen(game_start); i++)
        screen[START_TEXT_ROW][START_TEXT_COL + i] = (screenbuffer_t){game_start[i], 1};

    for(int i = 0; i < wcslen(game_replay); i++)
        screen[REPLAY_TEXT_ROW][REPLAY_TEXT_COL + i] = (screenbuffer_t){game_replay[i], 1};
}

void init_screen_buffer(wchar_t* player1, wchar_t* player2, int player1_time, int player2_time) {
    const wchar_t* files = L"A   B   C   D   E   F   G   H";
    const wchar_t* chess_board_line = L"+---+---+---+---+---+---+---+---+";
    const wchar_t* menu_line = L"+----------+----------+";
    const wchar_t* menu_line_final = L"+---------------------+";
    const wchar_t* game = L"game";
    const wchar_t* manual = L"manual";

    for (int i = 0; i < wcslen(files); i++)
        screen[0][4 + i] = (screenbuffer_t){files[i], 1};

    for (int i = 1; i <= 20; i += 2)
        for (int j = 0; j < wcslen(chess_board_line); j++)
            screen[i][2 + j] = (screenbuffer_t){chess_board_line[j], 1};

    for (int row = 0; row < 8; row++) {
        int y = 2 + row * 2;
        screen[y][0] = (screenbuffer_t){L'8' - row, 1};
        for (int col = 0; col < 8; col++) {
            int x = 2 + col * 4;
            screen[y][x] = (screenbuffer_t){L'|', 1};
            const wchar_t* uni = get_unicode_piece(chessboard[row][col]);
            screen[y][x + 2] = (screenbuffer_t){uni[0], 1};
        }
        screen[y][2 + 8 * 4] = (screenbuffer_t){L'|', 1};
    }

    screen[18][2] = (screenbuffer_t){L'|', 1};
    screen[18][34] = (screenbuffer_t){L'|', 1};

    for(int i = 0; i < wcslen(player1); i++)
        screen[18][3 + i] = (screenbuffer_t){player1[i], 1};

    for(int i = 0; i < wcslen(player2); i++)
        screen[18][27 + i] = (screenbuffer_t){player2[i], 1};

    for(int i = 0; i < SCREEN_HEIGHT; i++) {
        if(i == 0 || i == 2) {
            for(int j = 0; j < wcslen(menu_line); j++)
                screen[i][SCREEN_MENU_COL + j] = (screenbuffer_t){menu_line[j],1};
        } else if(i == 1) {
            screen[i][SCREEN_MENU_COL] = (screenbuffer_t){L'|', 1};
            screen[i][SCREEN_MENU_COL + 11] = (screenbuffer_t){L'|', 1};
            screen[i][SCREEN_MENU_COL + 22] = (screenbuffer_t){L'|', 1};
            for(int j = 0; j < wcslen(game); j++)
                screen[i][MENU_GAME_COL_FRONT + j + 3] = (screenbuffer_t){game[j],1};
            for(int j = 0; j < wcslen(manual); j++)
                screen[i][MENU_MANUAL_COL_FRONT + j] = (screenbuffer_t){manual[j],1};
        } else if(i == SCREEN_HEIGHT - 1) {
            for(int j = 0; j < wcslen(menu_line_final); j++)
                screen[i][SCREEN_MENU_COL + j] = (screenbuffer_t){menu_line_final[j],1};
        } else {
            screen[i][SCREEN_MENU_COL] = (screenbuffer_t){L'|', 1};
            screen[i][SCREEN_MENU_COL + 22] = (screenbuffer_t){L'|', 1};
        }
    }

    main_game_view();
}

void print_screen_buffer() {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            attron(COLOR_PAIR(screen[y][x].color_pair));
            mvaddnwstr(y, x, &screen[y][x].c, 1);
            attroff(COLOR_PAIR(screen[y][x].color_pair));
        }
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

int main() {
    setlocale(LC_ALL, "");
    initscr();
    noecho();
    cbreak();

    init_colors();
    clear_screen_buffer();
    init_chess_board();
    init_screen_buffer(L"player1", L"player2", 10, 10);
    print_screen_buffer();
    refresh();

    char c = getch();
    if(c == 'c') {
        clear_main_screen_buffer();
        print_screen_buffer();
        main_manual_view();
        print_screen_buffer();
    }

    c = getch();
    if(c == 'c') {
        clear_main_screen_buffer();
        print_screen_buffer();
        main_game_view();
        print_screen_buffer();
    }

    getch();
    endwin();
    free_pieces();
    return 0;
}