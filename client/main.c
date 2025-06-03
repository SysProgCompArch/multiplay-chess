#define _XOPEN_SOURCE_EXTENDED
#include <locale.h>
#include <ncursesw/ncurses.h>
#include <wchar.h>
#include <wctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "common.h"
#include "game_state.h"

// 화면 상태 enum
typedef enum
{
    SCREEN_MAIN,
    SCREEN_MATCHING,
    SCREEN_GAME
} screen_state_t;

// 클라이언트 상태 정보
typedef struct
{
    char username[32];
    char opponent_name[32];
    int socket_fd;
    bool connected;
    bool is_white;
    screen_state_t current_screen;
    time_t match_start_time;
    game_state_t game_state;
    int selected_x, selected_y; // 선택된 기물 위치
    bool piece_selected;        // 기물이 선택되었는지 여부
} client_state_t;

// 전역 변수
client_state_t g_client = {0};
pthread_mutex_t screen_mutex = PTHREAD_MUTEX_INITIALIZER;

// 보드 크기 및 위치 상수 재정의
#define SQUARE_W 7
#define SQUARE_H 3
#define BOARD_SIZE 8
#define BOARD_LABEL_X 4 // 좌우 라벨 여백
#define BOARD_LABEL_Y 2 // 상하 라벨 여백
#define BOARD_BORDER 2  // 테두리
#define BOARD_WIDTH (SQUARE_W * BOARD_SIZE + BOARD_LABEL_X * 2)
#define BOARD_HEIGHT (SQUARE_H * BOARD_SIZE + BOARD_LABEL_Y * 2)

// 메인 화면 UI 크기
#define MAIN_MENU_HEIGHT 15
#define MAIN_MENU_WIDTH 50
#define DIALOG_HEIGHT 8
#define DIALOG_WIDTH 40

// 게임 화면 UI 크기
#define CHAT_HEIGHT 16
#define CHAT_WIDTH 35
#define INPUT_HEIGHT 3

// 매칭 화면 UI 크기
#define MATCH_DIALOG_HEIGHT 10
#define MATCH_DIALOG_WIDTH 50

// 색상 쌍 정의
#define COLOR_PAIR_BORDER 1
#define COLOR_PAIR_SELECTED 2
#define COLOR_PAIR_WHITE_PIECE 3
#define COLOR_PAIR_BLACK_PIECE 4
#define COLOR_PAIR_BOARD_WHITE 5
#define COLOR_PAIR_BOARD_BLACK 6
#define COLOR_PAIR_SELECTED_SQUARE 7

// 함수 선언
void signal_handler(int signum);
void init_ncurses();
void cleanup_ncurses();
void draw_border(WINDOW *win);
void draw_main_screen();
void draw_matching_screen();
void draw_game_screen();
bool get_username_dialog();
void start_matching();
void cancel_matching();
void handle_mouse_input(MEVENT *event);
void update_match_timer();
void *network_thread(void *arg);
int connect_to_server();

// 체스 보드 관련
void draw_chess_board(WINDOW *board_win);
void draw_chat_area(WINDOW *chat_win);
void draw_game_menu(WINDOW *menu_win);
char get_piece_char(piece_t *piece, int team);
void handle_board_click(int board_x, int board_y);
bool coord_to_board_pos(int screen_x, int screen_y, int *board_x, int *board_y);

// 신호 처리기 - SIGINT 무시
void signal_handler(int signum)
{
    if (signum == SIGINT)
    {
        // SIGINT 무시 - 게임 중단 방지
        return;
    }
}

// ncurses 초기화
void init_ncurses()
{
    setlocale(LC_ALL, "");

    initscr();

    // 터미널 크기 먼저 확인
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    if (rows < 30 || cols < 107)
    {
        endwin();
        printf("터미널 크기가 너무 작습니다. 최소 %dx%d가 필요합니다.\n",
               107, 30);
        printf("현재 크기: %dx%d\n", cols, rows);
        exit(1);
    }

    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    mousemask(ALL_MOUSE_EVENTS, NULL);
    curs_set(0);

    // 색상 초기화
    if (has_colors())
    {
        start_color();
        init_pair(COLOR_PAIR_BORDER, COLOR_CYAN, COLOR_BLACK);
        init_pair(COLOR_PAIR_SELECTED, COLOR_BLACK, COLOR_WHITE);
        init_pair(COLOR_PAIR_WHITE_PIECE, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_PAIR_BLACK_PIECE, COLOR_YELLOW, COLOR_BLACK);
        init_pair(COLOR_PAIR_BOARD_WHITE, COLOR_BLACK, COLOR_WHITE);
        init_pair(COLOR_PAIR_BOARD_BLACK, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_PAIR_SELECTED_SQUARE, COLOR_RED, COLOR_YELLOW);
    }

    clear();
    refresh();
}

// ncurses 정리
void cleanup_ncurses()
{
    if (isendwin() == FALSE)
    {
        endwin();
    }
}

// 테두리 그리기
void draw_border(WINDOW *win)
{
    wattron(win, COLOR_PAIR(COLOR_PAIR_BORDER));
    box(win, 0, 0);
    wattroff(win, COLOR_PAIR(COLOR_PAIR_BORDER));
}

// 메인 화면 그리기
void draw_main_screen()
{
    clear();
    refresh();

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int start_y = (rows - MAIN_MENU_HEIGHT) / 2;
    int start_x = (cols - MAIN_MENU_WIDTH) / 2;

    WINDOW *main_win = newwin(MAIN_MENU_HEIGHT, MAIN_MENU_WIDTH, start_y, start_x);
    draw_border(main_win);

    mvwprintw(main_win, 2, (MAIN_MENU_WIDTH - 18) / 2, "MULTIPLAYER CHESS");
    mvwprintw(main_win, 4, (MAIN_MENU_WIDTH - 30) / 2, "==============================");
    mvwprintw(main_win, 7, (MAIN_MENU_WIDTH - 20) / 2, "1. Start Game");
    mvwprintw(main_win, 8, (MAIN_MENU_WIDTH - 20) / 2, "2. Options");
    mvwprintw(main_win, 9, (MAIN_MENU_WIDTH - 20) / 2, "3. Exit");
    mvwprintw(main_win, 12, (MAIN_MENU_WIDTH - 35) / 2, "Press 1, 2, 3 or use mouse");

    wrefresh(main_win);
}

// 사용자명 입력 다이얼로그
bool get_username_dialog()
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int start_y = (rows - DIALOG_HEIGHT) / 2;
    int start_x = (cols - DIALOG_WIDTH) / 2;

    WINDOW *dialog = newwin(DIALOG_HEIGHT, DIALOG_WIDTH, start_y, start_x);
    draw_border(dialog);

    mvwprintw(dialog, 2, (DIALOG_WIDTH - 20) / 2, "Enter your username:");
    mvwprintw(dialog, 4, 2, "Name: ");

    wrefresh(dialog);

    echo();
    curs_set(1);

    char input[32] = {0};
    mvwgetnstr(dialog, 4, 8, input, 30);

    noecho();
    curs_set(0);

    delwin(dialog);

    if (strlen(input) > 0)
    {
        strncpy(g_client.username, input, sizeof(g_client.username) - 1);
        strncpy(g_client.game_state.local_player, input, sizeof(g_client.game_state.local_player) - 1);
        return true;
    }

    return false;
}

// 매칭 화면 그리기
void draw_matching_screen()
{
    clear();
    refresh();

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int start_y = (rows - MATCH_DIALOG_HEIGHT) / 2;
    int start_x = (cols - MATCH_DIALOG_WIDTH) / 2;

    WINDOW *match_win = newwin(MATCH_DIALOG_HEIGHT, MATCH_DIALOG_WIDTH, start_y, start_x);
    draw_border(match_win);

    mvwprintw(match_win, 2, (MATCH_DIALOG_WIDTH - 20) / 2, "Finding opponent...");

    time_t current_time = time(NULL);
    int elapsed = (int)(current_time - g_client.match_start_time);
    int minutes = elapsed / 60;
    int seconds = elapsed % 60;

    mvwprintw(match_win, 4, (MATCH_DIALOG_WIDTH - 20) / 2, "Wait time: %02d:%02d", minutes, seconds);
    mvwprintw(match_win, 6, (MATCH_DIALOG_WIDTH - 25) / 2, "Searching for player...");
    mvwprintw(match_win, 8, (MATCH_DIALOG_WIDTH - 30) / 2, "Press ESC to cancel");

    wrefresh(match_win);
}

// 체스 말 ASCII 문자 반환
char get_piece_char(piece_t *piece, int team)
{
    if (!piece)
        return ' ';

    // 흰색 팀 (0) - 대문자, 검은색 팀 (1) - 소문자
    if (team == 0)
    { // 흰색
        switch (piece->type)
        {
        case KING:
            return 'K';
        case QUEEN:
            return 'Q';
        case ROOK:
            return 'R';
        case BISHOP:
            return 'B';
        case KNIGHT:
            return 'N';
        case PAWN:
            return 'P';
        }
    }
    else
    { // 검은색
        switch (piece->type)
        {
        case KING:
            return 'k';
        case QUEEN:
            return 'q';
        case ROOK:
            return 'r';
        case BISHOP:
            return 'b';
        case KNIGHT:
            return 'n';
        case PAWN:
            return 'p';
        }
    }
    return ' ';
}

// 화면 좌표를 보드 좌표로 변환
bool coord_to_board_pos(int screen_x, int screen_y, int *board_x, int *board_y)
{
    // 보드 윈도우 시작 위치 (draw_game_screen에서 newwin 위치)
    int board_win_start_x = 2;
    int board_win_start_y = 1;

    // 윈도우 내에서의 상대 좌표 계산
    int rel_x = screen_x - board_win_start_x;
    int rel_y = screen_y - board_win_start_y;

    // 체스판 시작 위치 (윈도우 내에서 BOARD_LABEL_X, 2 오프셋)
    int chess_start_x = BOARD_LABEL_X;
    int chess_start_y = 2;

    // 체스판 영역 내인지 확인
    if (rel_x < chess_start_x || rel_y < chess_start_y)
        return false;

    // 체스판 내 상대 좌표
    int chess_rel_x = rel_x - chess_start_x;
    int chess_rel_y = rel_y - chess_start_y;

    // 체스판 전체 크기 체크
    int total_chess_width = SQUARE_W * BOARD_SIZE;
    int total_chess_height = SQUARE_H * BOARD_SIZE;

    if (chess_rel_x >= total_chess_width || chess_rel_y >= total_chess_height)
        return false;

    // 어느 칸인지 계산
    *board_x = chess_rel_x / SQUARE_W;
    *board_y = chess_rel_y / SQUARE_H;

    // 범위 확인
    if (*board_x >= 0 && *board_x < BOARD_SIZE && *board_y >= 0 && *board_y < BOARD_SIZE)
    {
        return true;
    }

    return false;
}

// 보드 클릭 처리
void handle_board_click(int board_x, int board_y)
{
    if (g_client.current_screen != SCREEN_GAME)
        return;

    board_state_t *board = &g_client.game_state.board_state;

    if (!g_client.piece_selected)
    {
        // 기물 선택
        piecestate_t *piece = &board->board[board_y][board_x];
        if (piece->piece && !piece->is_dead)
        {
            // TODO: 현재 플레이어의 기물인지 확인
            g_client.selected_x = board_x;
            g_client.selected_y = board_y;
            g_client.piece_selected = true;

            add_chat_message(&g_client.game_state, "System",
                             "Piece selected. Click destination.");
        }
    }
    else
    {
        // 기물 이동
        if (board_x == g_client.selected_x && board_y == g_client.selected_y)
        {
            // 같은 위치 클릭 - 선택 해제
            g_client.piece_selected = false;
            add_chat_message(&g_client.game_state, "System", "Selection cancelled.");
        }
        else
        {
            // 다른 위치 클릭 - 이동 시도
            if (make_move(board, g_client.selected_x, g_client.selected_y, board_x, board_y))
            {
                add_chat_message(&g_client.game_state, "System", "Piece moved.");
                g_client.piece_selected = false;
                // TODO: 서버에 이동 정보 전송
            }
            else
            {
                add_chat_message(&g_client.game_state, "System", "Invalid move.");
            }
        }
    }
}

// 체스 보드 그리기
void draw_chess_board(WINDOW *board_win)
{
    werase(board_win);
    draw_border(board_win);

    // 상단 라벨 (테두리+라벨)
    mvwprintw(board_win, 1, BOARD_LABEL_Y, "     a      b      c      d      e      f      g      h");

    board_state_t *board = &g_client.game_state.board_state;

    for (int row = 0; row < BOARD_SIZE; row++)
    {
        // 왼쪽 라벨: 테두리 다음 칸(1)에 표시
        mvwprintw(board_win, 2 + row * SQUARE_H + 1, 1, "%d ", 8 - row);
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            int y = 2 + row * SQUARE_H;
            int x = BOARD_LABEL_X + col * SQUARE_W;
            bool is_selected = (g_client.piece_selected &&
                                g_client.selected_x == col &&
                                g_client.selected_y == row);
            // 색상
            if (is_selected)
            {
                wattron(board_win, COLOR_PAIR(COLOR_PAIR_SELECTED_SQUARE));
            }
            else if ((row + col) % 2 == 0)
            {
                wattron(board_win, COLOR_PAIR(COLOR_PAIR_BOARD_WHITE));
            }
            else
            {
                wattron(board_win, COLOR_PAIR(COLOR_PAIR_BOARD_BLACK));
            }
            // 3줄로 한 칸 출력
            mvwprintw(board_win, y, x, "       ");
            piecestate_t *piece = &board->board[row][col];
            if (piece->piece && !piece->is_dead)
            {
                char piece_char = get_piece_char(piece->piece, 0); // 임시로 흰색
                mvwprintw(board_win, y + 1, x, "   %c   ", piece_char);
            }
            else
            {
                mvwprintw(board_win, y + 1, x, "       ");
            }
            mvwprintw(board_win, y + 2, x, "       ");
            // 색상 해제
            if (is_selected)
            {
                wattroff(board_win, COLOR_PAIR(COLOR_PAIR_SELECTED_SQUARE));
            }
            else if ((row + col) % 2 == 0)
            {
                wattroff(board_win, COLOR_PAIR(COLOR_PAIR_BOARD_WHITE));
            }
            else
            {
                wattroff(board_win, COLOR_PAIR(COLOR_PAIR_BOARD_BLACK));
            }
        }
        // 오른쪽 라벨: 테두리 바로 안쪽(BOARD_WIDTH - 2)에 표시
        mvwprintw(board_win, 2 + row * SQUARE_H + 1, BOARD_WIDTH - 2, "%d", 8 - row);
    }
    // 하단 라벨
    mvwprintw(board_win, BOARD_HEIGHT - 2, BOARD_LABEL_X, "   a      b      c      d      e      f      g      h");
    wrefresh(board_win);
}

// 채팅 영역 그리기
void draw_chat_area(WINDOW *chat_win)
{
    werase(chat_win);
    draw_border(chat_win);

    mvwprintw(chat_win, 1, 2, "Chat");

    // 채팅 메시지 표시
    game_state_t *state = &g_client.game_state;
    int display_count = (state->chat_count > 12) ? 12 : state->chat_count;
    int start_index = (state->chat_count > 12) ? state->chat_count - 12 : 0;

    for (int i = 0; i < display_count; i++)
    {
        chat_message_t *msg = &state->chat_messages[start_index + i];
        mvwprintw(chat_win, 3 + i, 2, "%s: %s", msg->sender, msg->message);
    }

    wrefresh(chat_win);
}

// 게임 메뉴 그리기
void draw_game_menu(WINDOW *menu_win)
{
    werase(menu_win);
    draw_border(menu_win);

    mvwprintw(menu_win, 1, 2, "Game Menu");
    mvwprintw(menu_win, 3, 2, "1. Resign");
    mvwprintw(menu_win, 4, 2, "2. Draw");
    mvwprintw(menu_win, 5, 2, "3. Save");
    mvwprintw(menu_win, 6, 2, "4. Main");

    // 현재 턴 표시
    board_state_t *board = &g_client.game_state.board_state;
    const char *turn_text = (board->current_turn == TEAM_WHITE) ? "White" : "Black";
    mvwprintw(menu_win, 8, 2, "Turn: %s", turn_text);

    wrefresh(menu_win);
}

// 게임 화면 그리기
void draw_game_screen()
{
    clear();
    refresh();

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    WINDOW *board_win = newwin(BOARD_HEIGHT, BOARD_WIDTH, 1, 2);
    draw_chess_board(board_win);

    WINDOW *chat_win = newwin(CHAT_HEIGHT, CHAT_WIDTH, 1, BOARD_WIDTH + 5);
    draw_chat_area(chat_win);

    WINDOW *input_win = newwin(INPUT_HEIGHT, CHAT_WIDTH, CHAT_HEIGHT + 2, BOARD_WIDTH + 5);
    draw_border(input_win);
    mvwprintw(input_win, 1, 2, "Message: (Enter)");
    wrefresh(input_win);

    WINDOW *menu_win = newwin(10, 20, BOARD_HEIGHT + 2, 2);
    draw_game_menu(menu_win);

    mvprintw(0, 2, "Player: %s vs %s", g_client.username, g_client.opponent_name);
    mvprintw(0, cols - 20, "Status: Playing");

    wrefresh(board_win);
    wrefresh(chat_win);
    wrefresh(input_win);
    wrefresh(menu_win);
    refresh();
}

// 매칭 시작
void start_matching()
{
    g_client.current_screen = SCREEN_MATCHING;
    g_client.match_start_time = time(NULL);

    // TODO: 서버에 매칭 요청 전송
    // 현재는 시연용으로 1초 후 자동으로 게임 화면으로 이동
}

// 매칭 취소
void cancel_matching()
{
    g_client.current_screen = SCREEN_MAIN;
    // TODO: 서버에 매칭 취소 요청 전송
}

// 매칭 타이머 업데이트 (매초 호출)
void update_match_timer()
{
    if (g_client.current_screen == SCREEN_MATCHING)
    {
        // 시연용: 1초 후 자동으로 게임 시작
        time_t current_time = time(NULL);
        if (current_time - g_client.match_start_time >= 1)
        {
            strcpy(g_client.opponent_name, "Opponent");
            strcpy(g_client.game_state.opponent_player, "Opponent");
            g_client.is_white = true;
            g_client.game_state.local_team = TEAM_WHITE;
            g_client.game_state.game_in_progress = true;
            g_client.current_screen = SCREEN_GAME;

            // 게임 시작 메시지 추가
            add_chat_message(&g_client.game_state, "System", "Game started!");
            add_chat_message(&g_client.game_state, "System", "Click on pieces to select them.");
        }
    }
}

// 마우스 입력 처리
void handle_mouse_input(MEVENT *event)
{
    if (event->bstate & BUTTON1_CLICKED)
    {
        if (g_client.current_screen == SCREEN_GAME)
        {
            int board_x, board_y;
            if (coord_to_board_pos(event->x, event->y, &board_x, &board_y))
            {
                handle_board_click(board_x, board_y);
            }
        }
    }
}

// 서버 연결 (현재는 미구현)
int connect_to_server()
{
    // TODO: 실제 서버 연결 구현
    return -1;
}

// 네트워크 스레드 (현재는 미구현)
void *network_thread(void *arg)
{
    // TODO: 서버와의 통신 처리
    return NULL;
}

// 메인 함수
int main()
{
    // 신호 처리기 등록
    signal(SIGINT, signal_handler);

    // 클라이언트 상태 초기화
    g_client.current_screen = SCREEN_MAIN;
    g_client.connected = false;
    g_client.piece_selected = false;
    init_game_state(&g_client.game_state);

    // ncurses 초기화
    init_ncurses();

    // 시작 메시지
    add_chat_message(&g_client.game_state, "System", "Welcome to Chess Client!");

    // 첫 화면 그리기
    draw_main_screen();

    // 메인 게임 루프
    while (1)
    {
        // 입력 처리 (논블로킹)
        timeout(1000); // 1초 타임아웃
        int ch = getch();

        if (ch != ERR)
        {
            // 실제 키 입력이 있을 때만 처리
            if (ch == KEY_MOUSE)
            {
                MEVENT event;
                if (getmouse(&event) == OK)
                {
                    handle_mouse_input(&event);
                }
            }
            else
            {
                // 키보드 입력 처리
                switch (g_client.current_screen)
                {
                case SCREEN_MAIN:
                    switch (ch)
                    {
                    case '1':
                        if (get_username_dialog())
                        {
                            start_matching();
                        }
                        break;
                    case '2':
                        // TODO: 옵션 화면
                        add_chat_message(&g_client.game_state, "System", "Options feature coming soon.");
                        break;
                    case '3':
                    case 'q':
                    case 'Q':
                        cleanup_ncurses();
                        exit(0);
                        break;
                    }
                    break;

                case SCREEN_MATCHING:
                    switch (ch)
                    {
                    case 27: // ESC
                    case 'c':
                    case 'C':
                        cancel_matching();
                        break;
                    }
                    break;

                case SCREEN_GAME:
                    switch (ch)
                    {
                    case '1':
                        // 기권 처리
                        add_chat_message(&g_client.game_state, "System", "You resigned the game.");
                        g_client.current_screen = SCREEN_MAIN;
                        break;
                    case '2':
                        // 무승부 제안
                        add_chat_message(&g_client.game_state, g_client.username, "offers a draw.");
                        break;
                    case '3':
                        // 게임 저장
                        add_chat_message(&g_client.game_state, "System", "Game save feature coming soon.");
                        break;
                    case '4':
                    case 27: // ESC
                        g_client.current_screen = SCREEN_MAIN;
                        break;
                    case '\n':
                    case '\r':
                        // 채팅 입력 모드 (간단 구현)
                        add_chat_message(&g_client.game_state, g_client.username, "Hello!");
                        break;
                    }
                    break;
                }
            }
        }

        // 화면 업데이트 (매번 수행)
        pthread_mutex_lock(&screen_mutex);

        switch (g_client.current_screen)
        {
        case SCREEN_MAIN:
            draw_main_screen();
            break;
        case SCREEN_MATCHING:
            draw_matching_screen();
            update_match_timer();
            break;
        case SCREEN_GAME:
            draw_game_screen();
            break;
        }

        pthread_mutex_unlock(&screen_mutex);
    }

    cleanup_ncurses();
    return 0;
}