#include "logger.h"
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

static FILE *log_file = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static const char *log_level_strings[] = {
    "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

// 로거 초기화
int logger_init(const char *log_file_path)
{
    pthread_mutex_lock(&log_mutex);

    if (log_file != NULL)
    {
        fclose(log_file);
    }

    log_file = fopen(log_file_path, "a");
    if (log_file == NULL)
    {
        pthread_mutex_unlock(&log_mutex);
        return -1;
    }

    // 버퍼링 비활성화 (즉시 플러시)
    setbuf(log_file, NULL);

    pthread_mutex_unlock(&log_mutex);

    // 시작 로그 남기기
    LOG_INFO("=== Logger initialized ===");

    return 0;
}

// 로거 정리
void logger_cleanup(void)
{
    pthread_mutex_lock(&log_mutex);

    if (log_file != NULL)
    {
        LOG_INFO("=== Logger cleanup ===");
        fclose(log_file);
        log_file = NULL;
    }

    pthread_mutex_unlock(&log_mutex);
}

// 현재 시간을 문자열로 반환
static void get_timestamp(char *buffer, size_t size)
{
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", local_time);
}

// 로그 메시지 출력
void log_message(log_level_t level, const char *file, int line, const char *func, const char *format, ...)
{
    if (log_file == NULL)
    {
        return;
    }

    pthread_mutex_lock(&log_mutex);

    // 타임스탬프
    char timestamp[64];
    get_timestamp(timestamp, sizeof(timestamp));

    // 파일명에서 경로 제거
    const char *filename = strrchr(file, '/');
    if (filename == NULL)
    {
        filename = file;
    }
    else
    {
        filename++; // '/' 다음 문자부터
    }

    // 헤더 출력 [TIMESTAMP] [LEVEL] filename:line func() -
    fprintf(log_file, "[%s] [%s] %s:%d %s() - ",
            timestamp, log_level_strings[level], filename, line, func);

    // 실제 메시지 출력
    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);

    fprintf(log_file, "\n");

    pthread_mutex_unlock(&log_mutex);
}

// perror와 유사한 로그 함수
void log_perror(const char *message)
{
    LOG_ERROR("%s: %s", message, strerror(errno));
}