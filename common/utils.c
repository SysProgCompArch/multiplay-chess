// utils.c
#include "utils.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "piece.h"

static inline bool in_board(int x, int y) {
    return x >= 0 && x < 8 && y >= 0 && y < 8;
}

void clear_board(game_t* G) {
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            G->board[y][x].piece     = NULL;
            G->board[y][x].color     = TEAM_WHITE;
            G->board[y][x].is_dead   = 1;
            G->board[y][x].has_moved = false;
        }
}

void init_startpos(game_t* G) {
    // 표준 FEN: 흰작, 캐슬링 전부 가능, 앙파상 없음, 반수 0, 풀수 1
    fen_parse(G,
              "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR "
              "w KQkq - 0 1");
}

bool fen_parse(game_t* G, const char* fen) {
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
                        color_t      color = isupper(*p) ? TEAM_WHITE : TEAM_BLACK;
                        char         lc    = tolower(*p);
                        piece_type_t type;
                        switch (lc) {
                            case 'k':
                                type = PIECE_KING;
                                break;
                            case 'q':
                                type = PIECE_QUEEN;
                                break;
                            case 'r':
                                type = PIECE_ROOK;
                                break;
                            case 'b':
                                type = PIECE_BISHOP;
                                break;
                            case 'n':
                                type = PIECE_KNIGHT;
                                break;
                            case 'p':
                                type = PIECE_PAWN;
                                break;
                            default:
                                free(s);
                                return false;
                        }
                        piece_t* piece           = piece_table[color][type];
                        G->board[y][x].piece     = piece;
                        G->board[y][x].color     = color;
                        G->board[y][x].is_dead   = 0;
                        G->board[y][x].has_moved = false;
                        x++;
                    }
                }
                break;
            }
            case 1:  // side to move
                G->side_to_move = (tok[0] == 'b' ? TEAM_BLACK : TEAM_WHITE);
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
