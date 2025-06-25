#include "colors.h"

#include "ui.h"

void init_colors() {
    // 색상 모드 활성화
    start_color();

    init_pair(COLOR_PAIR_BOARD_WHITE, CHESS_LIGHT_SQUARE, CHESS_DARK_SQUARE);
    init_pair(COLOR_PAIR_BOARD_BLACK, CHESS_DARK_SQUARE, CHESS_LIGHT_SQUARE);

    // 색상 설정
    if (COLORS >= 256) {
        init_pair(COLOR_PAIR_BORDER, CHESS_BORDER, COLOR_BLACK);
        init_pair(COLOR_PAIR_DIALOG_BORDER, CHESS_DIALOG_BORDER, COLOR_BLACK);
        init_pair(COLOR_PAIR_SELECTED, EXT_COLOR_GOLD, COLOR_BLACK);
        init_pair(COLOR_PAIR_SELECTED_SQUARE, COLOR_BLACK, CHESS_SELECTED_SQUARE);
        init_pair(COLOR_PAIR_CURSOR_SQUARE, EXT_COLOR_GRAY_18, EXT_COLOR_GRAY_14);  // 연한 파란색 커서

        // 플레이어 정보 색상 쌍
        init_pair(COLOR_PAIR_WHITE_PLAYER, COLOR_BLACK, EXT_COLOR_BRIGHT_WHITE);  // 흰 팀: 검정 글자, 흰 배경
        init_pair(COLOR_PAIR_BLACK_PLAYER, EXT_COLOR_BRIGHT_WHITE, COLOR_BLACK);  // 검은 팀: 흰 글자, 검정 배경
        init_pair(COLOR_PAIR_CURRENT_TURN, EXT_COLOR_GOLD, COLOR_BLACK);          // 현재 턴: 금색 글자

        // 이동 가능 위치 색상 쌍
        init_pair(COLOR_PAIR_POSSIBLE_MOVE, COLOR_BLACK, CHESS_POSSIBLE_MOVE);  // 이동 가능: 옅은 초록 배경
        init_pair(COLOR_PAIR_CAPTURE_MOVE, COLOR_BLACK, CHESS_CAPTURE_MOVE);    // 캡처 가능: 옅은 붉은 배경

        // 기물 색상 쌍
        init_pair(COLOR_PAIR_MY_PIECE, EXT_COLOR_BRIGHT_WHITE, COLOR_BLACK);   // 내 기물: 밝은 흰색 (볼드로 표시됨)
        init_pair(COLOR_PAIR_OPPONENT_PIECE, EXT_COLOR_GRAY_14, COLOR_BLACK);  // 상대방 기물: 은은한 회색

        // 체크 상태 색상 쌍
        init_pair(COLOR_PAIR_RED, UI_ERROR, COLOR_BLACK);                      // 체크 상태: 빨간색 글자
        init_pair(COLOR_PAIR_CHECK_WHITE, UI_ERROR, EXT_COLOR_BRIGHT_WHITE);   // 흰 배경에서 체크 표시
        init_pair(COLOR_PAIR_CHECK_BLACK, UI_ERROR, COLOR_BLACK);              // 검은 배경에서 체크 표시
        init_pair(COLOR_PAIR_CHECK_PLAYER, EXT_COLOR_BRIGHT_WHITE, UI_ERROR);  // 체크 상태 플레이어 정보창
        init_pair(COLOR_PAIR_CHECK_KING, EXT_COLOR_BRIGHT_WHITE, 88);          // 체크 상태 킹 하이라이트 (확장 컬러 88 배경)

        // 타이머 색상 쌍
        init_pair(COLOR_PAIR_TIMER_WARNING, UI_WARNING, COLOR_BLACK);                   // 1분 이하: 주황색 글자, 검은 배경
        init_pair(COLOR_PAIR_TIMER_CRITICAL, COLOR_RED, COLOR_BLACK);                   // 30초 이하: 빨간색 글자, 검은 배경
        init_pair(COLOR_PAIR_TIMER_WARNING_WHITE, UI_WARNING, EXT_COLOR_BRIGHT_WHITE);  // 1분 이하: 주황색 글자, 흰 배경
        init_pair(COLOR_PAIR_TIMER_CRITICAL_WHITE, COLOR_RED, EXT_COLOR_BRIGHT_WHITE);  // 30초 이하: 빨간색 글자, 흰 배경
        init_pair(COLOR_PAIR_TIMER_WARNING_CHECK, UI_WARNING, COLOR_RED);               // 1분 이하: 주황색 글자, 빨간 배경
        init_pair(COLOR_PAIR_TIMER_CRITICAL_CHECK, COLOR_RED, COLOR_RED);               // 30초 이하: 빨간색 글자, 빨간 배경
    } else {
        init_pair(COLOR_PAIR_BORDER, COLOR_CYAN, COLOR_BLACK);
        init_pair(COLOR_PAIR_DIALOG_BORDER, COLOR_CYAN, COLOR_BLACK);
        init_pair(COLOR_PAIR_SELECTED, COLOR_YELLOW, COLOR_BLACK);
        init_pair(COLOR_PAIR_SELECTED_SQUARE, COLOR_BLACK, COLOR_YELLOW);
        init_pair(COLOR_PAIR_CURSOR_SQUARE, COLOR_BLUE, COLOR_CYAN);  // 파란색 커서 (기본 색상)

        // 플레이어 정보 색상 쌍 (기본 색상)
        init_pair(COLOR_PAIR_WHITE_PLAYER, COLOR_BLACK, COLOR_WHITE);   // 흰 팀: 검정 글자, 흰 배경
        init_pair(COLOR_PAIR_BLACK_PLAYER, COLOR_WHITE, COLOR_BLACK);   // 검은 팀: 흰 글자, 검정 배경
        init_pair(COLOR_PAIR_CURRENT_TURN, COLOR_YELLOW, COLOR_BLACK);  // 현재 턴: 노란 글자

        // 이동 가능 위치 색상 쌍 (기본 색상)
        init_pair(COLOR_PAIR_POSSIBLE_MOVE, COLOR_BLACK, COLOR_GREEN);  // 이동 가능: 초록 배경
        init_pair(COLOR_PAIR_CAPTURE_MOVE, COLOR_BLACK, COLOR_RED);     // 캡처 가능: 붉은 배경

        // 기물 색상 쌍 (기본 색상)
        init_pair(COLOR_PAIR_MY_PIECE, COLOR_WHITE, COLOR_BLACK);        // 내 기물: 흰색 (볼드로 표시됨)
        init_pair(COLOR_PAIR_OPPONENT_PIECE, COLOR_BLACK, COLOR_BLACK);  // 상대방 기물: 검은색 (약간 어둡게)

        // 체크 상태 색상 쌍 (기본 색상)
        init_pair(COLOR_PAIR_RED, COLOR_RED, COLOR_BLACK);           // 체크 상태: 빨간색 글자
        init_pair(COLOR_PAIR_CHECK_WHITE, COLOR_RED, COLOR_WHITE);   // 흰 배경에서 체크 표시
        init_pair(COLOR_PAIR_CHECK_BLACK, COLOR_RED, COLOR_BLACK);   // 검은 배경에서 체크 표시
        init_pair(COLOR_PAIR_CHECK_PLAYER, COLOR_WHITE, COLOR_RED);  // 체크 상태 플레이어 정보창
        init_pair(COLOR_PAIR_CHECK_KING, COLOR_WHITE, 88);           // 체크 상태 킹 하이라이트 (확장 컬러 88 배경)

        // 타이머 색상 쌍 (기본 색상)
        init_pair(COLOR_PAIR_TIMER_WARNING, COLOR_YELLOW, COLOR_BLACK);        // 1분 이하: 노란색 글자, 검은 배경
        init_pair(COLOR_PAIR_TIMER_CRITICAL, COLOR_RED, COLOR_BLACK);          // 30초 이하: 빨간색 글자, 검은 배경
        init_pair(COLOR_PAIR_TIMER_WARNING_WHITE, COLOR_YELLOW, COLOR_WHITE);  // 1분 이하: 노란색 글자, 흰 배경
        init_pair(COLOR_PAIR_TIMER_CRITICAL_WHITE, COLOR_RED, COLOR_WHITE);    // 30초 이하: 빨간색 글자, 흰 배경
        init_pair(COLOR_PAIR_TIMER_WARNING_CHECK, COLOR_YELLOW, COLOR_RED);    // 1분 이하: 노란색 글자, 빨간 배경
        init_pair(COLOR_PAIR_TIMER_CRITICAL_CHECK, COLOR_RED, COLOR_RED);      // 30초 이하: 빨간색 글자, 빨간 배경
    }
}