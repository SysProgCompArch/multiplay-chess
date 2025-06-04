// utils.c
#include "utils.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "piece.h"

static inline bool in_board(int x, int y) {
    return x >= 0 && x < 8 && y >= 0 && y < 8;
}

static void clear_board(game_t* G) {
    for (int x = 0; x < 8; x++)
        for (int y = 0; y < 8; y++) {
            G->board[x][y].piece         = NULL;
            G->board[x][y].is_dead       = 1;
            G->board[x][y].is_promoted   = 0;
            G->board[x][y].is_first_move = 0;
        }
}

void init_startpos(game_t* G) {
    // 표준 FEN: 흰작, 캐슬링 전부 가능, 앙파상 없음, 반수 0, 풀수 1
    fen_parse(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR "
        "w KQkq - 0 1",
        G);
}

bool fen_parse(const char* fen, game_t* G) {
    clear_board(G);
    // 토큰 분리: 6 필드 (piece, side, castling, ep, halfmove, fullmove)
    char* s = strdup(fen);
    if (!s) return false;
    char* tok = NULL;
    int   fld = 0;
    tok       = strtok(s, " ");
    while (tok && fld < 6) {
        switch (fld) {
            case 0: {  // piece placement
                int x = 0, y = 7;
                for (char* p = tok; *p && y >= 0; p++) {
                    if (*p == '/') {
                        y--;
                        x = 0;
                    } else if (isdigit(*p)) {
                        x += (*p - '0');
                    } else {
                        int         color = isupper(*p) ? WHITE : BLACK;
                        char        lc    = tolower(*p);
                        piecetype_t type;
                        switch (lc) {
                            case 'k':
                                type = KING;
                                break;
                            case 'q':
                                type = QUEEN;
                                break;
                            case 'r':
                                type = ROOK;
                                break;
                            case 'b':
                                type = BISHOP;
                                break;
                            case 'n':
                                type = KNIGHT;
                                break;
                            case 'p':
                                type = PAWN;
                                break;
                            default:
                                free(s);
                                return false;
                        }
                        G->board[x][y].piece         = piece_table[color][type];
                        G->board[x][y].color         = color;
                        G->board[x][y].is_dead       = 0;
                        G->board[x][y].is_promoted   = 0;
                        G->board[x][y].is_first_move = 1;
                        G->board[x][y].x             = x;
                        G->board[x][y].y             = y;
                        x++;
                    }
                }
                break;
            }
            case 1:  // side to move
                G->side_to_move = (tok[0] == 'b' ? BLACK : WHITE);
                break;
            case 2:  // castling rights
                G->white_can_castle_kingside  = strchr(tok, 'K') != NULL;
                G->white_can_castle_queenside = strchr(tok, 'Q') != NULL;
                G->black_can_castle_kingside  = strchr(tok, 'k') != NULL;
                G->black_can_castle_queenside = strchr(tok, 'q') != NULL;
                break;
            case 3:  // en passant
                if (tok[0] == '-') {
                    G->en_passant_x = G->en_passant_y = -1;
                } else if (isalpha(tok[0]) && isdigit(tok[1])) {
                    int file = tolower(tok[0]) - 'a';
                    int rank = tok[1] - '1';
                    if (in_board(file, rank)) {
                        G->en_passant_x = file;
                        G->en_passant_y = rank;
                    } else {
                        free(s);
                        return false;
                    }
                } else {
                    free(s);
                    return false;
                }
                break;
            case 4:  // halfmove clock
                G->halfmove_clock = atoi(tok);
                break;
                // case 5: fullmove number: 무시
        }
        fld++;
        tok = strtok(NULL, " ");
    }
    free(s);
    return (fld >= 5);
}
