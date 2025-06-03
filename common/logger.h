#ifndef COMMON_LOGGER_H
#define COMMON_LOGGER_H

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

// 로그 레벨 정의
typedef enum
{
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARN = 2,
    LOG_ERROR = 3,
    LOG_FATAL = 4
} log_level_t;

// 로그 출력 타입
typedef enum
{
    LOG_OUTPUT_FILE,   // 파일 출력 (클라이언트용)
    LOG_OUTPUT_CONSOLE // 콘솔 출력 (서버용)
} log_output_type_t;

// 로거 초기화 및 종료
int logger_init(log_output_type_t output_type, const char *file_path_or_prefix);
void logger_cleanup(void);

// 로그 함수들
void log_message(log_level_t level, const char *file, int line, const char *func, const char *format, ...);
void log_perror(const char *message);

// 편의 매크로들
#define LOG_DEBUG(...) log_message(LOG_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_INFO(...) log_message(LOG_INFO, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_WARN(...) log_message(LOG_WARN, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_ERROR(...) log_message(LOG_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_FATAL(...) log_message(LOG_FATAL, __FILE__, __LINE__, __func__, __VA_ARGS__)

#endif // COMMON_LOGGER_H