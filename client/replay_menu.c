// client/replay_menu.c

#include "replay_menu.h"  // void replay_menu(void);

#include <dirent.h>
#include <errno.h>
#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>  // usleep

#include "logger.h"  // LOG_INFO, LOG_ERROR, LOG_DEBUG
#include "replay.h"  // void start_replay(const char*);

void replay_menu(void) {
    LOG_INFO("replay_menu(): entered");

    // replays 디렉터리 없으면 생성 (존재하면 EEXIST 무시)
    if (mkdir("replays", 0755) && errno != EEXIST) {
        LOG_ERROR("replay_menu(): mkdir replays failed: %s", strerror(errno));
        return;
    }

    // 파일명 입력 윈도우 생성
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int dialog_width  = 60;
    int dialog_height = 10;
    int start_y       = (rows - dialog_height) / 2;
    int start_x       = (cols - dialog_width) / 2;

    WINDOW* dialog_win = newwin(dialog_height, dialog_width, start_y, start_x);
    keypad(dialog_win, TRUE);

    // 다이얼로그 그리기
    werase(dialog_win);
    box(dialog_win, 0, 0);
    mvwprintw(dialog_win, 0, 2, " Replay Menu ");
    mvwprintw(dialog_win, 2, 2, "Enter PGN filename (without .pgn extension):");
    mvwprintw(dialog_win, 3, 2, "(Leave empty to see available files)");
    mvwprintw(dialog_win, 5, 2, "Filename: ");
    mvwprintw(dialog_win, 7, 2, "Press Enter to confirm, ESC to cancel");

    // 입력 필드
    char filename[256] = "";
    int  cursor_pos    = 0;

    wrefresh(dialog_win);

    int ch;
    while (1) {
        // 입력 필드 업데이트
        mvwprintw(dialog_win, 5, 12, "%-40s", filename);
        wmove(dialog_win, 5, 12 + cursor_pos);
        wrefresh(dialog_win);

        ch = wgetch(dialog_win);

        if (ch == 27) {  // ESC
            LOG_INFO("replay_menu(): cancelled");
            break;
        } else if (ch == '\n') {
            if (strlen(filename) == 0) {
                // 빈 입력이면 파일 목록 보여주기
                delwin(dialog_win);
                show_replay_files();
                goto cleanup_and_exit;
            }

            // 파일 경로 생성
            char path[512];
            if (strstr(filename, ".pgn")) {
                snprintf(path, sizeof(path), "replays/%s", filename);
            } else {
                snprintf(path, sizeof(path), "replays/%s.pgn", filename);
            }

            // 파일 존재 확인
            FILE* fp = fopen(path, "r");
            if (fp) {
                fclose(fp);
                LOG_INFO("replay_menu(): start_replay(%s)", path);
                delwin(dialog_win);
                start_replay(path);
                goto cleanup_and_exit;
            } else {
                // 에러 표시
                mvwprintw(dialog_win, 6, 2, "Error: File not found!");
                wrefresh(dialog_win);
                usleep(1500000);                                        // 1.5초 대기
                mvwprintw(dialog_win, 6, 2, "                      ");  // 에러 메시지 지우기
            }
        } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (cursor_pos > 0) {
                filename[--cursor_pos] = '\0';
            }
        } else if (ch >= 32 && ch <= 126 && cursor_pos < 40) {
            // 출력 가능한 ASCII 문자
            filename[cursor_pos++] = ch;
            filename[cursor_pos]   = '\0';
        }
    }

    delwin(dialog_win);

cleanup_and_exit:
    // 화면 정리 및 메인 화면 복원
    clear();
    refresh();
}

// 파일 목록 보여주기 (빈 입력 시)
void show_replay_files(void) {
    DIR* dir = opendir("replays");
    if (!dir) {
        LOG_ERROR("show_replay_files(): could not open replays directory");
        return;
    }

    // .pgn 파일 목록 수집
    struct dirent* entry;
    char           files[100][256];
    int            count = 0;
    while ((entry = readdir(dir)) && count < 100) {
        if (strstr(entry->d_name, ".pgn")) {
            strncpy(files[count], entry->d_name, sizeof(files[count]) - 1);
            files[count][sizeof(files[count]) - 1] = '\0';
            count++;
        }
    }
    closedir(dir);

    if (count == 0) {
        LOG_INFO("show_replay_files(): no .pgn files found");
        return;
    }

    // 메뉴 윈도우 생성
    WINDOW* menu_win = newwin(count + 4, 50, 5, 10);
    keypad(menu_win, TRUE);

    int highlight = 0, ch;
    while (1) {
        werase(menu_win);
        box(menu_win, 0, 0);
        mvwprintw(menu_win, 0, 2, "Select Replay (Esc to cancel)");
        for (int i = 0; i < count; i++) {
            if (i == highlight) wattron(menu_win, A_REVERSE);
            mvwprintw(menu_win, i + 1, 2, "%s", files[i]);
            if (i == highlight) wattroff(menu_win, A_REVERSE);
        }
        wrefresh(menu_win);

        ch = wgetch(menu_win);
        LOG_DEBUG("show_replay_files(): key %d", ch);
        if (ch == KEY_UP) {
            highlight = (highlight + count - 1) % count;
        } else if (ch == KEY_DOWN) {
            highlight = (highlight + 1) % count;
        } else if (ch == '\n') {
            char path[512];
            snprintf(path, sizeof(path), "replays/%s", files[highlight]);
            LOG_INFO("show_replay_files(): start_replay(%s)", path);
            delwin(menu_win);
            start_replay(path);
            // 화면 정리 및 메인 화면 복원
            clear();
            refresh();
            return;
        } else if (ch == 27) {  // Esc
            LOG_INFO("show_replay_files(): cancelled");
            break;
        }
    }

    delwin(menu_win);
    // 화면 정리 및 메인 화면 복원
    clear();
    refresh();
}
