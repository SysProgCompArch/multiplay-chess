// replay.h
#ifndef REPLAY_H
#define REPLAY_H

#include <stdbool.h>

// 사용자에게 PGN(.pgn/.txt) 파일 경로를 입력받는 다이얼로그
// 성공 시 true, 취소 또는 오류 시 false
bool get_pgn_filepath_dialog(void);

// PGN 파일을 파싱하고, 순차적으로 화면에 재생하는 함수
void start_replay(void);

#endif // REPLAY_H
