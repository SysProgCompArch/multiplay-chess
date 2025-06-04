#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "handlers/match_manager.h"
#include "logger.h"
#include "server_network.h"

// 전역 변수로 epfd와 listener 저장 (시그널 핸들러에서 사용)
static int g_epfd     = -1;
static int g_listener = -1;

// 시그널 핸들러: 서버 종료 시 정리 작업
void cleanup_signal_handler(int signum) {
    LOG_INFO("Received signal %d, shutting down server...", signum);

    // 매칭 매니저 정리
    cleanup_match_manager();

    // 네트워크 리소스 정리
    if (g_listener >= 0)
        close(g_listener);
    if (g_epfd >= 0)
        close(g_epfd);

    LOG_INFO("Server shutdown complete");
    logger_cleanup();
    exit(0);
}

int main(int argc, char *argv[]) {
    // 로거 초기화 (다른 초기화보다 먼저) - 콘솔 출력 모드
    if (logger_init(LOG_OUTPUT_CONSOLE, NULL) != 0) {
        fprintf(stderr, "Failed to initialize logger\n");
        return 1;
    }

    LOG_INFO("Chess server starting... (PID: %d)", getpid());

    // 시그널 핸들러 등록
    signal(SIGINT, cleanup_signal_handler);
    signal(SIGTERM, cleanup_signal_handler);
    LOG_DEBUG("Signal handlers registered");

    // 매칭 매니저 초기화
    if (init_match_manager() < 0) {
        LOG_FATAL("Failed to initialize match manager");
        logger_cleanup();
        return 1;
    }

    int port = parse_port_from_args(argc, argv);
    LOG_INFO("Parsed port: %d", port);

    g_listener = create_and_bind_listener(port);
    LOG_INFO("Listener socket created and bound to port %d", port);

    g_epfd = setup_epoll(g_listener);
    LOG_INFO("Epoll instance created and listener registered");

    LOG_INFO("Chess server started successfully (port: %d)", port);
    LOG_INFO("Match manager initialized - ready for connections");

    event_loop(g_listener, g_epfd);

    // 정상 종료 시에도 정리 작업 수행
    cleanup_match_manager();
    cleanup(g_listener, g_epfd);
    logger_cleanup();

    return 0;
}
