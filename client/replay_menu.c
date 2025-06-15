// client/replay_menu.c

#include <ncurses.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>

#include "logger.h"        // LOG_INFO, LOG_ERROR, LOG_DEBUG
#include "replay_menu.h"   // void replay_menu(void);
#include "replay.h"        // void start_replay(const char*);

void replay_menu(void) {
    LOG_INFO("replay_menu(): entered");

    // replays 디렉터리 없으면 생성 (존재하면 EEXIST 무시)
    if (mkdir("replays", 0755) && errno != EEXIST) {
        LOG_ERROR("replay_menu(): mkdir replays failed: %s", strerror(errno));
        return;
    }

    // 디렉터리 열기
    DIR* dir = opendir("replays");
    if (!dir) {
        LOG_ERROR("replay_menu(): could not open replays directory");
        return;
    }

    // .pgn 파일 목록 수집
    struct dirent* entry;
    char files[100][256];
    int count = 0;
    while ((entry = readdir(dir)) && count < 100) {
        if (strstr(entry->d_name, ".pgn")) {
            strncpy(files[count], entry->d_name, sizeof(files[count]) - 1);
            files[count][sizeof(files[count]) - 1] = '\0';
            count++;
        }
    }
    closedir(dir);

    if (count == 0) {
        LOG_INFO("replay_menu(): no .pgn files found");
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
        LOG_DEBUG("replay_menu(): key %d", ch);
        if (ch == KEY_UP) {
            highlight = (highlight + count - 1) % count;
        } else if (ch == KEY_DOWN) {
            highlight = (highlight + 1) % count;
        } else if (ch == '\n') {
            char path[512];
            snprintf(path, sizeof(path), "replays/%s", files[highlight]);
            LOG_INFO("replay_menu(): start_replay(%s)", path);
            start_replay(path);
            break;
        } else if (ch == 27) {  // Esc
            LOG_INFO("replay_menu(): cancelled");
            break;
        }
    }

    delwin(menu_win);
}
