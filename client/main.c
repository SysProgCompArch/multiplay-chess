#define _XOPEN_SOURCE_EXTENDED
#include <locale.h>
#include <ncursesw/ncurses.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "client_state.h"
#include "client_network.h"
#include "ui.h"
#include "logger.h"

// 신호 처리기 - SIGINT 무시
void signal_handler(int signum)
{
    if (signum == SIGINT)
    {
        // SIGINT 무시 - 게임 중단 방지
        LOG_INFO("SIGINT ignored to prevent game interruption");
        return;
    }
}

// 메인 함수
int main()
{
    // 로거 초기화 (ncurses 초기화 전에)
    char log_filename[256];
    snprintf(log_filename, sizeof(log_filename), "chess_client_%d.log", getpid());

    if (logger_init(log_filename) != 0)
    {
        fprintf(stderr, "Failed to initialize logger\n");
        return 1;
    }

    LOG_INFO("Chess client starting... (PID: %d)", getpid());

    // 신호 처리기 등록
    signal(SIGINT, signal_handler);
    LOG_DEBUG("Signal handler registered");

    // 클라이언트 상태 초기화
    init_client_state();
    LOG_DEBUG("Client state initialized");

    // ncurses 초기화
    init_ncurses();
    LOG_INFO("ncurses initialized");

    // 시작 메시지
    add_chat_message_safe("System", "Welcome to Chess Client!");

    // 네트워크 스레드 시작
    network_thread_running = true;
    if (pthread_create(&network_thread_id, NULL, network_thread, NULL) != 0)
    {
        LOG_FATAL("Failed to create network thread");
        log_perror("pthread_create");
        cleanup_ncurses();
        logger_cleanup();
        exit(1);
    }
    LOG_INFO("Network thread started");

    // 첫 화면 그리기
    draw_main_screen();

    client_state_t *client = get_client_state();
    LOG_INFO("Starting main game loop");

    // 메인 게임 루프
    while (1)
    {
        // 입력 처리 (논블로킹)
        timeout(1000); // 1초 타임아웃
        int ch = getch();

        if (ch != ERR)
        {
            LOG_DEBUG("Input received: %d", ch);
            // 실제 키 입력이 있을 때만 처리
            if (ch == KEY_MOUSE)
            {
                MEVENT event;
                if (getmouse(&event) == OK)
                {
                    LOG_DEBUG("Mouse event: x=%d, y=%d, bstate=%08lx", event.x, event.y, event.bstate);
                    handle_mouse_input(&event);
                }
            }
            else
            {
                // 키보드 입력 처리
                switch (client->current_screen)
                {
                case SCREEN_MAIN:
                    switch (ch)
                    {
                    case '1':
                        LOG_INFO("User selected start matching");
                        if (get_username_dialog())
                        {
                            start_matching();
                        }
                        break;
                    case '2':
                        // TODO: 옵션 화면
                        LOG_INFO("User selected options (not implemented)");
                        add_chat_message_safe("System", "Options feature coming soon.");
                        break;
                    case '3':
                    case 'q':
                    case 'Q':
                        LOG_INFO("User requested quit");
                        cleanup_network();
                        cleanup_ncurses();
                        logger_cleanup();
                        exit(0);
                        break;
                    }
                    break;

                case SCREEN_MATCHING:
                    switch (ch)
                    {
                    case 27: // ESC
                    case 'c':
                    case 'C':
                        LOG_INFO("User cancelled matching");
                        cancel_matching();
                        break;
                    }
                    break;

                case SCREEN_GAME:
                    switch (ch)
                    {
                    case '1':
                        // 기권 처리
                        LOG_INFO("User resigned the game");
                        add_chat_message_safe("System", "You resigned the game.");
                        client->current_screen = SCREEN_MAIN;
                        break;
                    case '2':
                        // 무승부 제안
                        LOG_INFO("User offered a draw");
                        add_chat_message_safe(client->username, "offers a draw.");
                        break;
                    case '3':
                        // 게임 저장
                        LOG_INFO("User requested game save (not implemented)");
                        add_chat_message_safe("System", "Game save feature coming soon.");
                        break;
                    case '4':
                    case 27: // ESC
                        LOG_INFO("User returned to main screen from game");
                        client->current_screen = SCREEN_MAIN;
                        break;
                    case '\n':
                    case '\r':
                        // 채팅 입력 모드 (간단 구현)
                        LOG_DEBUG("User sent chat message");
                        add_chat_message_safe(client->username, "Hello!");
                        break;
                    }
                    break;
                }
            }
        }

        // 화면 업데이트 (매번 수행)
        pthread_mutex_lock(&screen_mutex);

        switch (client->current_screen)
        {
        case SCREEN_MAIN:
            draw_main_screen();
            break;
        case SCREEN_MATCHING:
            draw_matching_screen();
            update_match_timer();
            break;
        case SCREEN_GAME:
            draw_game_screen();
            break;
        }

        pthread_mutex_unlock(&screen_mutex);
    }

    cleanup_network();
    cleanup_ncurses();
    logger_cleanup();
    return 0;
}