#include "logger.h"

#include <pthread.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

// ANSI 색상 코드 정의
#define RESET  "\033[0m"
#define CYAN   "\033[96m"  // 타임스탬프
#define PURPLE "\033[95m"  // 파일명:라인번호
#define RED    "\033[91m"  // ERROR, FATAL
#define YELLOW "\033[93m"  // WARN
#define GREEN  "\033[92m"  // INFO
#define BLUE   "\033[94m"  // DEBUG
#define GRAY   "\033[90m"  // 함수명

static FILE             *log_file            = NULL;
static log_output_type_t output_type         = LOG_OUTPUT_CONSOLE;
static pthread_mutex_t   log_mutex           = PTHREAD_MUTEX_INITIALIZER;
static const char       *log_level_strings[] = {
    "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

// 로거 초기화
int logger_init(log_output_type_t type, const char *file_path_or_prefix) {
    pthread_mutex_lock(&log_mutex);

    output_type = type;

    if (output_type == LOG_OUTPUT_FILE) {
        // 파일 출력 모드 (클라이언트용)
        if (log_file != NULL) {
            fclose(log_file);
        }

        // logs 디렉토리 생성 (이미 존재하면 무시)
        mkdir("logs", 0755);

        char log_filename[256];
        if (file_path_or_prefix) {
            snprintf(log_filename, sizeof(log_filename), "logs/%s_%d.log", file_path_or_prefix, getpid());
        } else {
            snprintf(log_filename, sizeof(log_filename), "logs/app_%d.log", getpid());
        }

        log_file = fopen(log_filename, "a");
        if (log_file == NULL) {
            pthread_mutex_unlock(&log_mutex);
            return -1;
        }

        // 버퍼링 비활성화 (즉시 플러시)
        setbuf(log_file, NULL);

        pthread_mutex_unlock(&log_mutex);
        LOG_INFO("=== Client Logger initialized (file: %s) ===", log_filename);
    } else {
        // 콘솔 출력 모드 (서버용)
        log_file = NULL;
        pthread_mutex_unlock(&log_mutex);
        LOG_INFO("=== Server Logger initialized (console output) ===");
    }

    return 0;
}

// 로거 정리
void logger_cleanup(void) {
    pthread_mutex_lock(&log_mutex);

    if (output_type == LOG_OUTPUT_FILE && log_file != NULL) {
        // 뮤텍스를 잠근 상태에서 LOG_INFO 호출하면 데드락 발생
        // 대신 직접 파일에 작성
        if (log_file) {
            fprintf(log_file, "[%s] [INFO] logger.c:%d logger_cleanup() - === Logger cleanup ===\n",
                    "cleanup", __LINE__);
        }
        fclose(log_file);
        log_file = NULL;
    } else if (output_type == LOG_OUTPUT_CONSOLE) {
        // 콘솔의 경우도 직접 출력
        printf("=== Server Logger cleanup ===\n");
        fflush(stdout);
    }

    pthread_mutex_unlock(&log_mutex);
}

// 현재 시간을 문자열로 반환
static void get_timestamp(char *buffer, size_t size) {
    time_t     now        = time(NULL);
    struct tm *local_time = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", local_time);
}

// 콘솔 출력용 색상 적용
static void print_colored_console_log(log_level_t level, const char *timestamp, const char *filename,
                                      int line, const char *func, const char *message) {
    const char *level_color;

    switch (level) {
        case LOG_DEBUG:
            level_color = BLUE;
            break;
        case LOG_INFO:
            level_color = GREEN;
            break;
        case LOG_WARN:
            level_color = YELLOW;
            break;
        case LOG_ERROR:
        case LOG_FATAL:
            level_color = RED;
            break;
        default:
            level_color = RESET;
    }

    printf("%s[%s]%s %s[%s]%s %s%s:%d%s %s%s()%s - %s\n",
           CYAN, timestamp, RESET,                        // 타임스탬프
           level_color, log_level_strings[level], RESET,  // 로그 레벨
           PURPLE, filename, line, RESET,                 // 파일명:라인번호
           GRAY, func, RESET,                             // 함수명
           message                                        // 메시지
    );
    fflush(stdout);
}

// 로그 메시지 출력
void log_message(log_level_t level, const char *file, int line, const char *func, const char *format, ...) {
    pthread_mutex_lock(&log_mutex);

    // 타임스탬프
    char timestamp[64];
    get_timestamp(timestamp, sizeof(timestamp));

    // 파일명에서 경로 제거
    const char *filename = strrchr(file, '/');
    if (filename == NULL) {
        filename = file;
    } else {
        filename++;  // '/' 다음 문자부터
    }

    // 메시지 포맷팅
    char    message[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    if (output_type == LOG_OUTPUT_FILE && log_file != NULL) {
        // 파일 출력 (클라이언트용)
        fprintf(log_file, "[%s] [%s] %s:%d %s() - %s\n",
                timestamp, log_level_strings[level], filename, line, func, message);
    } else if (output_type == LOG_OUTPUT_CONSOLE) {
        // 콘솔 출력 (서버용, 색상 적용)
        print_colored_console_log(level, timestamp, filename, line, func, message);
    }

    pthread_mutex_unlock(&log_mutex);
}

// perror와 유사한 로그 함수
void log_perror(const char *message) {
    LOG_ERROR("%s: %s", message, strerror(errno));
}