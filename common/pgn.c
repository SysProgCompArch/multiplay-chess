#include "pgn.h"
#include "pgn.h" 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool san_pgn_load(const char* filename, game_moves_t* out) {
    FILE* f = fopen(filename, "r");
    if (!f) return false;

    // ���� ��ü �б�
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = malloc(sz + 1);
    fread(buf, 1, sz, f);
    buf[sz] = '\0'; fclose(f);

    // ���([..]) �ǳʶٰ� �� �� �� �ؽ�Ʈ ����
    char* p = buf;
    while (*p) {
        if (p[0] == '\n' && p[1] != '[') { p++; break; }
        p++;
    }

    // ��ūȭ
    const char* delim = " \r\n\t";
    char* tok = strtok(p, delim);
    int cap = 128;
    move_t* arr = malloc(sizeof(move_t) * cap);
    int cnt = 0;

    // �ӽ� ���� ����: ������ġ
    game_t G;
    init_startpos(&G);

    while (tok) {
        // ����ȣ/�������
        if (strchr(tok, '.') || strcmp(tok, "1-0") == 0
            || strcmp(tok, "0-1") == 0
            || strcmp(tok, "1/2-1/2") == 0) {
            tok = strtok(NULL, delim);
            continue;
        }

        int sx, sy, dx, dy;
        piecetype_t promo;
        if (!san_to_move(&G, tok, &sx, &sy, &dx, &dy, &promo)) {
            fprintf(stderr, "Invalid SAN: %s\n", tok);
            free(buf); free(arr);
            return false;
        }

        // �迭�� ����
        if (cnt >= cap) {
            cap *= 2;
            arr = realloc(arr, sizeof(move_t) * cap);
        }
        arr[cnt++] = (move_t){ sx,sy,dx,dy,promo };

        // ���� ���� ������Ʈ
        apply_move(&G, sx, sy, dx, dy);

        tok = strtok(NULL, delim);
    }
    free(buf);

    out->moves = arr;
    out->count = cnt;
    return true;
}

void san_pgn_free(game_moves_t* gm) {
    free(gm->moves);
    gm->moves = NULL;
    gm->count = 0;
}
