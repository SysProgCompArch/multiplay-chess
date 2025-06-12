// client/replay.c

#include <ncurses.h>            // ncurses 기본
#include <limits.h>             // PATH_MAX
#include <string.h>             // strlen, strncpy
#include <unistd.h>             // usleep
#include <dirent.h>      // opendir, readdir
#include <stdlib.h>

#include "ui/ui.h"                 // draw_border(), DIALOG_WIDTH 등
#include "replay.h"
#include "client_state.h"    // get_client_state(), client_state_t
#include "game_state.h"      // init_game_state(), apply_move_from_server(), etc.
#include "pgn.h"             // parse_pgn_file(), pgn_free()
#include "logger.h"   // LOG_INFO, LOG_ERROR

extern void draw_current_screen(void);

bool get_pgn_filepath_dialog(void) {
    // 1) replay 디렉토리에서 .pgn 파일 목록 읽기
    DIR *dp = opendir("replay");
    if (!dp){
        return false;
    }
    char **files = NULL;
    int   nfiles = 0;
    struct dirent *ent;
    while ((ent = readdir(dp))) {
        int len = strlen(ent->d_name);
        if (len > 4 && strcmp(ent->d_name + len - 4, ".pgn") == 0) {
            files = realloc(files, sizeof(char*)*(nfiles+1));
            files[nfiles] = strdup(ent->d_name);
            nfiles++;
        }
    }
    closedir(dp);
    if (nfiles == 0) {
        free(files);
        return false;
    }

    // 2) 다이얼로그 윈도우 생성
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int h = DIALOG_HEIGHT, w = DIALOG_WIDTH;
    int y = (rows - h)/2, x = (cols - w)/2;
    WINDOW *dlg = newwin(h, w, y, x);
    keypad(dlg, TRUE);
    curs_set(0);

    // 3) 스크롤 변수
    int choice = 0, top = 0;
    int visible = h - 4;  // 상하 테두리+타이틀 제외

    // 4) 입력 루프
    int ch;
    while (1) {
        werase(dlg);
        draw_border(dlg);
        mvwprintw(dlg, 1, 2, "Select PGN file (ESC=cancel)");

        // 파일 리스트 출력
        for (int i = 0; i < visible; i++) {
            int idx = top + i;
            if (idx >= nfiles) break;
            if (idx == choice) wattron(dlg, A_REVERSE);
            mvwprintw(dlg, 2 + i, 2, "%2d. %s", idx+1, files[idx]);
            if (idx == choice) wattroff(dlg, A_REVERSE);
        }
        wrefresh(dlg);

        ch = wgetch(dlg);
        if (ch == KEY_UP) {
            if (choice > 0) choice--;
            if (choice < top) top = choice;
        }
        else if (ch == KEY_DOWN) {
            if (choice < nfiles-1) choice++;
            if (choice >= top + visible) top = choice - visible +1;
        }
        else if (ch == 10 /*Enter*/ ) {
            // 선택 확정
            char path[PATH_MAX];
            snprintf(path, sizeof(path), "replay/%s", files[choice]);
            client_state_t *cli = get_client_state();
            strncpy(cli->pgn_filepath, path, PATH_MAX-1);
            cli->pgn_filepath[PATH_MAX-1] = '\0';
            break;
        }
        else if (ch == 27 /*ESC*/ ) {
            // 취소
            choice = -1;
            break;
        }
    }

    // 5) 정리
    delwin(dlg);
    for (int i = 0; i < nfiles; i++) free(files[i]);
    free(files);

    return (choice >= 0);
}

void draw_replay_chat(WINDOW *chat_win, client_state_t *cli);

void start_replay(void) {
    client_state_t *cli = get_client_state();
    LOG_INFO("Interactive replay from '%s'", cli->pgn_filepath);

    // 1) 이전 리플레이 데이터 해제 & 재초기화
    pgn_free(&cli->replay_pgn);
    pgn_init(&cli->replay_pgn);
   // 2) PGN 파싱 (직접 cli->replay_pgn 에)
    if (parse_pgn_file(cli->pgn_filepath, &cli->replay_pgn) != 0) {
        add_chat_message_safe("System", "Replay: 파일 로드 실패");
        draw_current_screen(); refresh();
        return;
    }

    // 2) client_state 에 저장
    cli->replay_index = 0;
    cli->replay_mode  = true;

    // 3) 보드 초기화
    game_state_t *st = &cli->game_state;
    init_game_state(st);
    reset_game_to_starting_position(&st->game);

    // 4) ncurses 설정
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);

    // 5) 첫 프레임: 보드+현재 수 표시
    cli->current_screen = SCREEN_GAME;
    cli->screen_update_requested = true;
    clear(); draw_current_screen(); refresh();


    // 6) 인터랙티브 루프
    int ch;
    while (1) {
        ch = getch();
        if (ch == 'q') break;
        bool moved = false;
        if (ch == KEY_RIGHT && cli->replay_index < cli->replay_pgn.move_count) {
            // 앞으로 한 수
            char *san = cli->replay_pgn.moves[cli->replay_index].san;
            char from[3] = {san[0], san[1], '\0'};
            char to  [3] = {san[2], san[3], '\0'};
            apply_move_from_server(&st->game, from, to);
            cli->replay_index++;
            moved = true;
        }
        else if (ch == KEY_LEFT && cli->replay_index > 0) {
            // 뒤로 한 수: 보드 리셋 & 0~index-1 재생
            cli->replay_index--;
            init_game_state(st);
            reset_game_to_starting_position(&st->game);
            for (size_t i = 0; i < cli->replay_index; i++) {
                char *san = cli->replay_pgn.moves[i].san;
                char from[3] = {san[0], san[1], '\0'};
                char to  [3] = {san[2], san[3], '\0'};
                apply_move_from_server(&st->game, from, to);
            }
            moved = true;
        }

        draw_current_screen();
        refresh();

        usleep(50000);
    }

    // 7) 종료
    cli->replay_mode = false;
    pgn_free(&cli->replay_pgn);
    add_chat_message_safe("System", "Replay 종료");
    draw_current_screen(); refresh();

    // 키 모드 복원
    nodelay(stdscr, TRUE);
    keypad(stdscr, FALSE);
}