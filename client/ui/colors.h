#ifndef EXTENDED_COLORS_H
#define EXTENDED_COLORS_H

// ncurses 256 색상 팔레트
// 0-15: 기본 16색
// 16-231: 6x6x6 RGB 색상 큐브
// 232-255: 그레이스케일

// 기본 16색 (표준 ncurses 색상)
#define EXT_COLOR_BLACK          0
#define EXT_COLOR_RED            1
#define EXT_COLOR_GREEN          2
#define EXT_COLOR_YELLOW         3
#define EXT_COLOR_BLUE           4
#define EXT_COLOR_MAGENTA        5
#define EXT_COLOR_CYAN           6
#define EXT_COLOR_WHITE          7
#define EXT_COLOR_BRIGHT_BLACK   8
#define EXT_COLOR_BRIGHT_RED     9
#define EXT_COLOR_BRIGHT_GREEN   10
#define EXT_COLOR_BRIGHT_YELLOW  11
#define EXT_COLOR_BRIGHT_BLUE    12
#define EXT_COLOR_BRIGHT_MAGENTA 13
#define EXT_COLOR_BRIGHT_CYAN    14
#define EXT_COLOR_BRIGHT_WHITE   15

// 6x6x6 RGB 색상 큐브 (16-231)
// 각 색상 요소는 0-5 범위의 값을 가짐
// 색상 번호 = 16 + 36*r + 6*g + b (r,g,b는 0-5)
#define RGB_COLOR(r, g, b) (16 + 36 * (r) + 6 * (g) + (b))

// 자주 사용되는 RGB 색상들
#define EXT_COLOR_MAROON       RGB_COLOR(1, 0, 0)
#define EXT_COLOR_DARK_GREEN   RGB_COLOR(0, 1, 0)
#define EXT_COLOR_BROWN        RGB_COLOR(2, 1, 0)
#define EXT_COLOR_NAVY         RGB_COLOR(0, 0, 1)
#define EXT_COLOR_PURPLE       RGB_COLOR(1, 0, 1)
#define EXT_COLOR_TEAL         RGB_COLOR(0, 1, 1)
#define EXT_COLOR_ORANGE       RGB_COLOR(5, 2, 0)
#define EXT_COLOR_PINK         RGB_COLOR(5, 2, 4)
#define EXT_COLOR_LIME         RGB_COLOR(2, 5, 0)
#define EXT_COLOR_VIOLET       RGB_COLOR(2, 0, 5)
#define EXT_COLOR_GOLD         RGB_COLOR(5, 5, 0)
#define EXT_COLOR_SILVER       RGB_COLOR(3, 3, 3)
#define EXT_COLOR_LIGHT_BLUE   RGB_COLOR(2, 4, 5)
#define EXT_COLOR_LIGHT_GREEN  RGB_COLOR(2, 5, 2)
#define EXT_COLOR_LIGHT_RED    RGB_COLOR(5, 2, 2)
#define EXT_COLOR_LIGHT_YELLOW RGB_COLOR(5, 5, 2)

// 그레이스케일 (232-255)
// 0에서 23까지의 그레이 레벨
#define GRAY_COLOR(level) (232 + (level))

#define EXT_COLOR_GRAY_0  232  // 가장 어두운 회색
#define EXT_COLOR_GRAY_1  233
#define EXT_COLOR_GRAY_2  234
#define EXT_COLOR_GRAY_3  235
#define EXT_COLOR_GRAY_4  236
#define EXT_COLOR_GRAY_5  237
#define EXT_COLOR_GRAY_6  238
#define EXT_COLOR_GRAY_7  239
#define EXT_COLOR_GRAY_8  240
#define EXT_COLOR_GRAY_9  241
#define EXT_COLOR_GRAY_10 242
#define EXT_COLOR_GRAY_11 243
#define EXT_COLOR_GRAY_12 244
#define EXT_COLOR_GRAY_13 245
#define EXT_COLOR_GRAY_14 246
#define EXT_COLOR_GRAY_15 247
#define EXT_COLOR_GRAY_16 248
#define EXT_COLOR_GRAY_17 249
#define EXT_COLOR_GRAY_18 250
#define EXT_COLOR_GRAY_19 251
#define EXT_COLOR_GRAY_20 252
#define EXT_COLOR_GRAY_21 253
#define EXT_COLOR_GRAY_22 254
#define EXT_COLOR_GRAY_23 255  // 가장 밝은 회색

// 체스 게임에 유용한 색상들
#define CHESS_LIGHT_SQUARE    EXT_COLOR_WHITE
#define CHESS_DARK_SQUARE     EXT_COLOR_BLACK
#define CHESS_SELECTED_SQUARE RGB_COLOR(5, 3, 0)  // 짙은 오렌지색
#define CHESS_POSSIBLE_MOVE   RGB_COLOR(0, 3, 0)  // 연한 초록색
#define CHESS_CAPTURE_MOVE    RGB_COLOR(3, 0, 0)  // 연한 빨간색
#define CHESS_CHECK_HIGHLIGHT RGB_COLOR(5, 1, 1)  // 밝은 빨간색
#define CHESS_BORDER          EXT_COLOR_GRAY_16
#define CHESS_DIALOG_BORDER   EXT_COLOR_GRAY_20

// UI 요소에 유용한 색상들
#define UI_BUTTON_NORMAL  RGB_COLOR(3, 3, 3)  // 중간 회색
#define UI_BUTTON_HOVER   RGB_COLOR(4, 4, 4)  // 밝은 회색
#define UI_BUTTON_PRESSED RGB_COLOR(2, 2, 2)  // 어두운 회색
#define UI_TEXT_PRIMARY   EXT_COLOR_BRIGHT_WHITE
#define UI_TEXT_SECONDARY EXT_COLOR_GRAY_18
#define UI_TEXT_DISABLED  EXT_COLOR_GRAY_12
#define UI_BACKGROUND     EXT_COLOR_BLACK
#define UI_BORDER         RGB_COLOR(1, 1, 3)  // 어두운 파란색
#define UI_ACCENT         RGB_COLOR(0, 3, 5)  // 파란색
#define UI_SUCCESS        RGB_COLOR(0, 4, 0)  // 초록색
#define UI_WARNING        RGB_COLOR(5, 3, 0)  // 주황색
#define UI_ERROR          88

// 색상 매크로 함수들
#define IS_EXTENDED_COLOR(c) ((c) >= 16 && (c) <= 255)
#define IS_RGB_COLOR(c)      ((c) >= 16 && (c) <= 231)
#define IS_GRAY_COLOR(c)     ((c) >= 232 && (c) <= 255)

// RGB 색상에서 각 요소 추출
#define RGB_RED_COMPONENT(c)   (((c)-16) / 36)
#define RGB_GREEN_COMPONENT(c) ((((c)-16) % 36) / 6)
#define RGB_BLUE_COMPONENT(c)  (((c)-16) % 6)

// 그레이 레벨 추출
#define GRAY_LEVEL(c) ((c)-232)

// 색상 쌍 정의
#define COLOR_PAIR_BORDER          1
#define COLOR_PAIR_DIALOG_BORDER   2
#define COLOR_PAIR_SELECTED        3
#define COLOR_PAIR_BOARD_WHITE     4
#define COLOR_PAIR_BOARD_BLACK     5
#define COLOR_PAIR_SELECTED_SQUARE 6
#define COLOR_PAIR_CURSOR_SQUARE   7
#define COLOR_PAIR_WHITE_PLAYER    8
#define COLOR_PAIR_BLACK_PLAYER    9
#define COLOR_PAIR_CURRENT_TURN    10
#define COLOR_PAIR_POSSIBLE_MOVE   11
#define COLOR_PAIR_CAPTURE_MOVE    12
#define COLOR_PAIR_MY_PIECE        13
#define COLOR_PAIR_OPPONENT_PIECE  14
#define COLOR_PAIR_RED             15  // 체크 상태 표시용
#define COLOR_PAIR_CHECK_WHITE     16  // 흰색 플레이어 배경에서의 체크 표시
#define COLOR_PAIR_CHECK_BLACK     17  // 검은색 플레이어 배경에서의 체크 표시
#define COLOR_PAIR_CHECK_PLAYER    18  // 체크 상태 플레이어 정보창 (흰 글자 + 빨간 배경)
#define COLOR_PAIR_CHECK_KING      19  // 체크 상태 킹 하이라이트 (확장 컬러 88 배경)

#endif  // EXTENDED_COLORS_H