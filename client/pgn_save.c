// common/pgn_save.c
#include "pgn_save.h"
#include <stdio.h>
#include <time.h>

int write_pgn(const char* filename, game_state_t* state) {
    FILE* fp = fopen(filename, "w");
    if (!fp) return -1;

    // 날짜 생성
    time_t t = time(NULL);
    struct tm tm_info;
    localtime_r(&t, &tm_info);
    char date[11];
    strftime(date, sizeof(date), "%Y.%m.%d", &tm_info);

    // PGN 헤더
    fprintf(fp, "[Event \"Multiplay Chess\"]\n");
    fprintf(fp, "[Site \"Local\"]\n");
    fprintf(fp, "[Date \"%s\"]\n", date);
    fprintf(fp, "[Round \"1\"]\n");
    // White/Black 태그: local_team에 따라 이름 매핑
    if (state->local_team == TEAM_WHITE) {
        fprintf(fp, "[White \"%s\"]\n", state->local_player);
        fprintf(fp, "[Black \"%s\"]\n", state->opponent_player);
    } else {
        fprintf(fp, "[White \"%s\"]\n", state->opponent_player);
        fprintf(fp, "[Black \"%s\"]\n", state->local_player);
    }
    fprintf(fp, "[Result \"%s\"]\n\n", state->pgn_result);

    // LAN(예: e2e4) 형식의 수 출력
    for (int i = 0; i < state->pgn_move_count; ++i) {
        if (i % 2 == 0)
            fprintf(fp, "%d. %s", i/2+1, state->pgn_moves[i]);
        else
            fprintf(fp, " %s\n", state->pgn_moves[i]);
    }
    if (state->pgn_move_count % 2 != 0)
        fprintf(fp, "\n");

    fclose(fp);
    return 0;
}
