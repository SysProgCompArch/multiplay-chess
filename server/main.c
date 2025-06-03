#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "server_network.h"
#include "handlers/match_manager.h"

// 전역 변수로 epfd와 listener 저장 (시그널 핸들러에서 사용)
static int g_epfd = -1;
static int g_listener = -1;

// 시그널 핸들러: 서버 종료 시 정리 작업
void cleanup_signal_handler(int signum)
{
    printf("\nReceived signal %d, shutting down server...\n", signum);

    // 매칭 매니저 정리
    cleanup_match_manager();

    // 네트워크 리소스 정리
    if (g_listener >= 0)
        close(g_listener);
    if (g_epfd >= 0)
        close(g_epfd);

    printf("Server shutdown complete\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    // 시그널 핸들러 등록
    signal(SIGINT, cleanup_signal_handler);
    signal(SIGTERM, cleanup_signal_handler);

    // 매칭 매니저 초기화
    if (init_match_manager() < 0)
    {
        fprintf(stderr, "Failed to initialize match manager\n");
        return 1;
    }

    int port = parse_port_from_args(argc, argv);
    g_listener = create_and_bind_listener(port);
    g_epfd = setup_epoll(g_listener);

    printf("Chess server started (port: %d)\n", port);
    printf("Match manager initialized - ready for connections\n");

    event_loop(g_listener, g_epfd);

    // 정상 종료 시에도 정리 작업 수행
    cleanup_match_manager();
    cleanup(g_listener, g_epfd);

    return 0;
}
