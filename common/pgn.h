#ifndef PGN_H
#define PGN_H

#include <stdbool.h>
#include "rule.h"
#include "utils.h"  

typedef struct {
	int sx, sy;            
	int dx, dy;            
	piecetype_t promotion;
} move_t;

typedef struct {
	move_t* moves;  
	int     count;
} game_moves_t;

bool san_pgn_load(const char* filename, game_moves_t* out_moves);
void san_pgn_free(game_moves_t* gm);

#endif // PGN_H
