// pgn.c
#include "pgn.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>    // stat, struct stat
#include <unistd.h>      // for POSIX functions
#include "logger.h"     // LOG_INFO, LOG_ERROR


#define INITIAL_MOVE_CAPACITY  16   // 초기 moves 배열 크기
#define MAX_LINE              512   // 한 줄 최대 길이



// 중복 회피용 유니크 파일명 생성 함수
static void make_unique_filename(char *buf, size_t bufsize) {
    struct stat st;
    if (stat(buf, &st) != 0) return;  // 존재하지 않으면 그대로

    // 확장자 분리
    char base[256], ext[32];
    char *dot = strrchr(buf, '.');
    if (dot) {
        size_t n = dot - buf;
        if (n >= sizeof(base)) n = sizeof(base)-1;
        strncpy(base, buf, n);
        base[n] = '\0';
        strncpy(ext, dot, sizeof(ext)-1);
        ext[sizeof(ext)-1] = '\0';
    } else {
        strncpy(base, buf, sizeof(base)-1);
        base[sizeof(base)-1] = '\0';
        ext[0] = '\0';
    }

    for (int i = 1; i < 1000; i++) {
        snprintf(buf, bufsize, "%s(%d)%s", base, i, ext);
        if (stat(buf, &st) != 0) return;  // 새로운 이름 발견
    }
}

// -------------------------------------------------------------
// PGNGame 초기화
// - 태그 필드(date, round, white, black)와 winner 초기화
// - moves 배열 malloc, count 및 capacity 설정
// -------------------------------------------------------------
void pgn_init(PGNGame *g) {
    g->date          = NULL;
    g->round         = NULL;
    g->white         = NULL;
    g->black         = NULL;

    g->moves         = malloc(sizeof(PGNMove) * INITIAL_MOVE_CAPACITY);
    if (!g->moves) { perror("malloc"); exit(1); }
    g->move_capacity = INITIAL_MOVE_CAPACITY;
    g->move_count    = 0;

    g->winner        = WINNER_UNKNOWN;
}

// -------------------------------------------------------------
// PGNGame 해제
// - 동적 할당된 문자열과 배열을 모두 free()
// -------------------------------------------------------------
void pgn_free(PGNGame *g) {
    // 1) 태그 문자열 해제
    free(g->date);    g->date    = NULL;
    free(g->round);   g->round   = NULL;
    free(g->white);   g->white   = NULL;
    free(g->black);   g->black   = NULL;

    // 2) 각 MoveEntry의 san 문자열 해제
    for (size_t i = 0; i < g->move_count; i++) {
        free(g->moves[i].san);
    }
    // 3) moves 배열 자체 해제
    free(g->moves);

    // 4) 해제 후 포인터와 카운트를 초기화
    g->moves         = NULL;
    g->move_count    = 0;
    g->move_capacity = 0;

    // 5) 승자 정보도 초기화
    g->winner        = WINNER_UNKNOWN;
}

// -------------------------------------------------------------
// moves 배열 크기 자동 확장
// - 현재 count >= capacity가 되면 capacity *= 2, realloc()
// -------------------------------------------------------------
static void ensure_move_capacity(PGNGame *g) {
    if (g->move_count >= g->move_capacity) {
        g->move_capacity *= 2;
        PGNMove *tmp = realloc(g->moves, sizeof(PGNMove) * g->move_capacity);
        if (!tmp) { perror("realloc"); exit(1); }
        g->moves = tmp;
    }
}

// -------------------------------------------------------------
// PGN 파일 파싱
// - 빈 줄을 기준으로 헤더와 본문(move) 구분
// - 헤더(tag): Date, Round, White, Black 태그만 처리
// - 본문(move): 수(move) 저장, 결과 토큰으로 winner 설정
// -------------------------------------------------------------
int parse_pgn_file(const char *filename, PGNGame *g) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("fopen");
        return -1;
    }

    char line[MAX_LINE];
    int  in_moves = 0;   // 0=헤더 모드, 1=수(move) 모드

    // 구조체 초기화
    pgn_init(g);

    while (fgets(line, MAX_LINE, fp)) {
        // 빈 줄(줄 길이 <=1)은 헤더→수 구분
        if (!in_moves && strlen(line) <= 1) {
            in_moves = 1;
            continue;
        }

        if (!in_moves) {
            // ─── 헤더(tag) 파싱 ───────────────────────────
            // "[Name \"Value\"]" 형식
            char name[16], val[256];
            if (sscanf(line, "[%15s \"%255[^\"]\"]", name, val) == 2) {
                // name 끝에 붙은 ']' 제거
                if (name[strlen(name)-1] == ']')
                    name[strlen(name)-1] = '\0';

                // 각 태그별로 strdup 처리
                if (strcmp(name, "Date") == 0) {
                    free(g->date);
                    g->date = strdup(val);
                } else if (strcmp(name, "Round") == 0) {
                    free(g->round);
                    g->round = strdup(val);
                } else if (strcmp(name, "White") == 0) {
                    free(g->white);
                    g->white = strdup(val);
                } else if (strcmp(name, "Black") == 0) {
                    free(g->black);
                    g->black = strdup(val);
                }
                // 그 외 태그는 무시
            }
        } else {
            // ─── 본문(move) 파싱 ───────────────────────────
            // 공백, 탭, 개행문자로 토큰 분리
            char *tok = strtok(line, " \t\r\n");
            while (tok) {
                // 토큰이 '1.' 같은 수 번호, '{' 주석, '(' 변형 시작이면 무시
                if (strchr(tok, '.') || tok[0]=='{' || tok[0]=='(') {
                    // 결과 토큰인 경우 winner 설정
                    if (strcmp(tok, "1-0") == 0) {
                        g->winner = WINNER_WHITE;
                    } else if (strcmp(tok, "0-1") == 0) {
                        g->winner = WINNER_BLACK;
                    } else if (strcmp(tok, "1/2-1/2") == 0) {
                        g->winner = WINNER_DRAW;
                    }
                } else {
                    // 실제 수(move) 토큰: 배열에 저장
                    ensure_move_capacity(g);
                    g->moves[g->move_count].san = strdup(tok);
                    g->move_count++;
                }
                tok = strtok(NULL, " \t\r\n");
            }
        }
    }

    fclose(fp);
    return 0;
}

// -------------------------------------------------------------
// save_pgn_file()
//  파일명 중복 시 "(1)", "(2)"... 붙여 유니크하게 생성하도록 수정
// -------------------------------------------------------------
int save_pgn_file(const char *orig_filename, const PGNGame *g) {
    char filename[256];
    strncpy(filename, orig_filename, sizeof(filename)-1);
    filename[sizeof(filename)-1] = '\0';

    // 1) 파일명 중복 회피
    make_unique_filename(filename, sizeof(filename));

    // 2) 파일 열기
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("fopen");
        return -1;
    }

    // 3) 헤더 태그 출력
    if (g->date)    fprintf(fp, "[Date \"%s\"]\n", g->date);
    else            fprintf(fp, "[Date \"????.??.??\"]\n");

    if (g->round)   fprintf(fp, "[Round \"%s\"]\n", g->round);
    else            fprintf(fp, "[Round \"?\"]\n");

    if (g->white)   fprintf(fp, "[White \"%s\"]\n", g->white);
    else            fprintf(fp, "[White \"?\"]\n");

    if (g->black)   fprintf(fp, "[Black \"%s\"]\n", g->black);
    else            fprintf(fp, "[Black \"?\"]\n");

    fprintf(fp, "\n");

    // 4) 수순 출력
    for (size_t i = 0; i < g->move_count; i++) {
        if (i % 2 == 0) fprintf(fp, "%zu. ", i/2 + 1);
        fprintf(fp, "%s", g->moves[i].san);
        if (i + 1 < g->move_count) fprintf(fp, " ");
    }

    // 5) 결과 추가
    switch (g->winner) {
        case WINNER_WHITE: fprintf(fp, " 1-0"); break;
        case WINNER_BLACK: fprintf(fp, " 0-1"); break;
        case WINNER_DRAW:  fprintf(fp, " 1/2-1/2"); break;
        default: break;
    }
    fprintf(fp, "\n");

    fclose(fp);

    LOG_INFO("PGN saved to %s", filename);
    return 0;
}