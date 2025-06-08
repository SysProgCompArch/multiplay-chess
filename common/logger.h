#ifndef COMMON_LOGGER_H
#define COMMON_LOGGER_H

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// 로그 레벨 정의
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO  = 1,
    LOG_WARN  = 2,
    LOG_ERROR = 3,
    LOG_FATAL = 4
} log_level_t;

// 로그 출력 타입
typedef enum {
    LOG_OUTPUT_FILE,    // 파일 출력 (클라이언트용)
    LOG_OUTPUT_CONSOLE  // 콘솔 출력 (서버용)
} log_output_type_t;

// 모듈별 로깅을 위한 모듈 ID
typedef enum {
    LOG_MODULE_MAIN = 0,
    LOG_MODULE_NETWORK,
    LOG_MODULE_UI,
    LOG_MODULE_GAME,
    LOG_MODULE_PROTOCOL,
    LOG_MODULE_DATABASE,
    LOG_MODULE_MATCH,
    LOG_MODULE_COUNT
} log_module_t;

// 로거 초기화 및 종료
int  logger_init(log_output_type_t output_type, const char *file_path_or_prefix);
void logger_cleanup(void);

// 로그 함수들
void log_message(log_level_t level, const char *file, int line, const char *func, const char *format, ...);
void log_perror(const char *message);

// 모듈별 로그 레벨 설정
void        logger_set_module_level(log_module_t module, log_level_t level);
log_level_t logger_get_module_level(log_module_t module);

// 모듈별 로그 함수
void log_module_message(log_module_t module, log_level_t level, const char *file, int line, const char *func, const char *format, ...);

// 편의 매크로들
#define LOG_DEBUG(...) log_message(LOG_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_INFO(...)  log_message(LOG_INFO, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_WARN(...)  log_message(LOG_WARN, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_ERROR(...) log_message(LOG_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_FATAL(...) log_message(LOG_FATAL, __FILE__, __LINE__, __func__, __VA_ARGS__)

// 성능 측정을 위한 매크로
#define LOG_PERF_START(name)            \
    struct timespec _perf_start_##name; \
    clock_gettime(CLOCK_MONOTONIC, &_perf_start_##name);

#define LOG_PERF_END(name, module)                                                                       \
    do {                                                                                                 \
        struct timespec _perf_end;                                                                       \
        clock_gettime(CLOCK_MONOTONIC, &_perf_end);                                                      \
        long _perf_diff = (_perf_end.tv_sec - _perf_start_##name.tv_sec) * 1000000000L +                 \
                          (_perf_end.tv_nsec - _perf_start_##name.tv_nsec);                              \
        log_module_message(module, LOG_DEBUG, __FILE__, __LINE__, __func__,                              \
                           "PERF: %s took %ld ns (%.3f ms)", #name, _perf_diff, _perf_diff / 1000000.0); \
    } while (0)

// 모듈별 편의 매크로들
#define LOG_NETWORK_DEBUG(...) log_module_message(LOG_MODULE_NETWORK, LOG_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_NETWORK_INFO(...)  log_module_message(LOG_MODULE_NETWORK, LOG_INFO, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_NETWORK_WARN(...)  log_module_message(LOG_MODULE_NETWORK, LOG_WARN, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_NETWORK_ERROR(...) log_module_message(LOG_MODULE_NETWORK, LOG_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define LOG_UI_DEBUG(...) log_module_message(LOG_MODULE_UI, LOG_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_UI_INFO(...)  log_module_message(LOG_MODULE_UI, LOG_INFO, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_UI_WARN(...)  log_module_message(LOG_MODULE_UI, LOG_WARN, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_UI_ERROR(...) log_module_message(LOG_MODULE_UI, LOG_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define LOG_GAME_DEBUG(...) log_module_message(LOG_MODULE_GAME, LOG_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_GAME_INFO(...)  log_module_message(LOG_MODULE_GAME, LOG_INFO, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_GAME_WARN(...)  log_module_message(LOG_MODULE_GAME, LOG_WARN, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_GAME_ERROR(...) log_module_message(LOG_MODULE_GAME, LOG_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)

#endif  // COMMON_LOGGER_H