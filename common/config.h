#ifndef COMMON_CONFIG_H
#define COMMON_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

// ===================================================================
// 공통 설정 관리 시스템
// ===================================================================

// 설정 타입
typedef enum {
    CONFIG_TYPE_STRING,
    CONFIG_TYPE_INT,
    CONFIG_TYPE_BOOL,
    CONFIG_TYPE_FLOAT
} config_type_t;

// 설정 항목 구조체
typedef struct {
    const char   *key;            // 설정 키
    config_type_t type;           // 설정 타입
    void         *value;          // 설정 값 포인터
    void         *default_value;  // 기본값 포인터
    const char   *description;    // 설정 설명
} config_item_t;

// 기본 네트워크 설정
#define DEFAULT_SERVER_HOST        "127.0.0.1"
#define DEFAULT_SERVER_PORT        8080
#define DEFAULT_RECONNECT_INTERVAL 5
#define DEFAULT_CONNECTION_TIMEOUT 10

// 기본 게임 설정
#define DEFAULT_GAME_TIME_LIMIT     600  // 10분
#define DEFAULT_MAX_CHAT_MESSAGES   50
#define DEFAULT_CHAT_MESSAGE_LENGTH 256

// 기본 UI 설정
#define DEFAULT_ANIMATION_ENABLED true
#define DEFAULT_SOUND_ENABLED     false
#define DEFAULT_AUTO_SAVE_ENABLED true

// 설정 관리 함수들
int  config_init(const char *config_file_path);
void config_cleanup(void);

// 설정 값 읽기
const char *config_get_string(const char *key, const char *default_value);
int         config_get_int(const char *key, int default_value);
bool        config_get_bool(const char *key, bool default_value);
float       config_get_float(const char *key, float default_value);

// 설정 값 쓰기
int config_set_string(const char *key, const char *value);
int config_set_int(const char *key, int value);
int config_set_bool(const char *key, bool value);
int config_set_float(const char *key, float value);

// 설정 파일 저장/로드
int config_save(void);
int config_load(void);

// 설정 초기화 (기본값으로 복원)
void config_reset_to_defaults(void);

// 설정 목록 출력 (디버그용)
void config_print_all(void);

#endif  // COMMON_CONFIG_H