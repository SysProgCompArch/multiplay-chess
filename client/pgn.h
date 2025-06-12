// pgn.h
#ifndef PGN_H
#define PGN_H

#include <stddef.h>

// 승자 정보를 위한 열거형(Enum)
// WINNER_UNKNOWN: 아직 결정되지 않음
// WINNER_WHITE  : 백(White) 승리
// WINNER_BLACK  : 흑(Black) 승리
// WINNER_DRAW   : 무승부
typedef enum {
    WINNER_UNKNOWN,
    WINNER_WHITE,
    WINNER_BLACK,
    WINNER_DRAW
} Winner;

// 한 수(move)를 저장하는 구조체
// san: Standard Algebraic Notation (예: "e4", "Nf3")
typedef struct {
    char *san;     // 동적 할당된 문자열 포인터
} PGNMove;

// 전체 PGN 게임 정보를 담는 구조체
typedef struct {
    // ──────────────────────────────────────
    // 고정 태그 (헤더 정보)
    // 각 태그는 모두 strdup()로 동적 할당하며
    // 사용 후 pgn_free()에서 free() 해 줘야 함
    // ──────────────────────────────────────
    char *date;    // [Date "YYYY.MM.DD"] 형식의 날짜 문자열
    char *round;   // [Round "n"] 형식의 라운드 번호
    char *white;   // [White "Player Name"] 백 플레이어 이름
    char *black;   // [Black "Player Name"] 흑 플레이어 이름

    // ──────────────────────────────────────
    // 수(move) 목록을 위한 동적 배열
    // moves         : PGNMove 구조체 배열
    // move_count    : 실제 저장된 수의 개수
    // move_capacity : 현재 할당된 배열 크기
    // ──────────────────────────────────────
    PGNMove *moves;
    size_t   move_count;
    size_t   move_capacity;

    // ──────────────────────────────────────
    // 대국 승자 정보
    // parse_pgn_file() 도중 "1-0", "0-1", "1/2-1/2" 토큰을 만나면 설정
    // 기본값: WINNER_UNKNOWN
    // ──────────────────────────────────────
    Winner   winner;
} PGNGame;

// PGNGame 구조체 초기화 함수
// - date, round, white, black 포인터를 NULL로 설정
// - moves 배열을 INITIAL_MOVE_CAPACITY만큼 malloc
// - move_count = 0, move_capacity = INITIAL_MOVE_CAPACITY
// - winner = WINNER_UNKNOWN
void    pgn_init(PGNGame *g);

// PGNGame 구조체 해제 함수
// - date, round, white, black, moves[i].san을 모두 free()
// - moves 배열 자체도 free()
// 반드시 parse_pgn_file 이후 호출해야 메모리 누수 방지
void    pgn_free(PGNGame *g);

// PGN 파일 파싱 함수
// filename: 읽을 PGN 파일 경로
// g       : pgn_init()된 PGNGame 포인터
// 반환값: 0=성공, -1=오픈 실패 등의 오류
//   - 헤더(tag)와 빈 줄을 기준으로 본문(move) 구분
//   - date, round, white, black 태그만 처리, 나머지는 무시
//   - 수(move) 토큰은 모두 moves[]에 저장
//   - "1-0"/"0-1"/"1/2-1/2" 토큰을 만나면 winner 설정
int     parse_pgn_file(const char *filename, PGNGame *g);


// PGNGame 내용을 PGN 형식 파일로 저장
// filename: 저장할 파일 경로
// g       : 저장할 PGNGame 구조체
// 리턴   :  0 = 성공, -1 = 오류
int     save_pgn_file(const char *filename, const PGNGame *g);


#endif  // PGN_H
