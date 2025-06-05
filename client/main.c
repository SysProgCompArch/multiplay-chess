#define _XOPEN_SOURCE_EXTENDED
#include <locale.h>
#include <ncursesw/ncurses.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "client_network.h"
#include "client_state.h"
#include "logger.h"
#include "ui/ui.h"

// 터미널 크기 변경 플래그
volatile sig_atomic_t terminal_resized = 0;

// 신호 처리기 - SIGINT 무시, SIGWINCH 처리
void signal_handler(int signum) {
    if (signum == SIGINT) {
        // SIGINT 무시 - 게임 중단 방지
        LOG_INFO("SIGINT ignored to prevent game interruption");
        return;
    } else if (signum == SIGWINCH) {
        // 터미널 크기 변경 신호
        terminal_resized = 1;
        LOG_DEBUG("Terminal resize signal received");
    }
}

// 메인 함수
int main() {
    // 로거 초기화 (ncurses 초기화 전에) - 파일 출력 모드
    if (logger_init(LOG_OUTPUT_FILE, "chess_client") != 0) {
        fprintf(stderr, "Failed to initialize logger\n");
        return 1;
    }

    LOG_INFO("Chess client starting... (PID: %d)", getpid());

    // 신호 처리기 등록
    signal(SIGINT, signal_handler);
    signal(SIGWINCH, signal_handler);
    LOG_DEBUG("Signal handlers registered (SIGINT, SIGWINCH)");

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
    if (pthread_create(&network_thread_id, NULL, network_thread, NULL) != 0) {
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
    while (1) {
        // 터미널 크기 변경 확인
        if (terminal_resized) {
            terminal_resized = 0;
            LOG_INFO("Handling terminal resize");
            handle_terminal_resize();
        }

        // 입력 처리 (논블로킹)
        timeout(1000);  // 1초 타임아웃
        int ch = getch();

        // 에러 다이얼로그가 활성화되어 있으면 키 입력 처리를 건너뜀
        pthread_mutex_lock(&screen_mutex);
        bool dialog_active = client->error_dialog_active;
        pthread_mutex_unlock(&screen_mutex);

        if (ch != ERR && !dialog_active) {
            LOG_DEBUG("Input received: %d", ch);
            // 실제 키 입력이 있을 때만 처리
            if (ch == KEY_MOUSE) {
                MEVENT event;
                if (getmouse(&event) == OK) {
                    LOG_DEBUG("Mouse event: x=%d, y=%d, bstate=%08lx", event.x, event.y, event.bstate);
                    handle_mouse_input(&event);
                }
            } else {
                // 키보드 입력 처리
                switch (client->current_screen) {
                    case SCREEN_MAIN:
                        switch (ch) {
                            case '1':
                                LOG_INFO("User selected start matching");
                                if (get_username_dialog()) {
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
                        switch (ch) {
                            case 27:  // ESC
                            case 'c':
                            case 'C':
                                LOG_INFO("User cancelled matching");
                                cancel_matching();
                                break;
                        }
                        break;

                    case SCREEN_GAME:
                        switch (ch) {
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
                            case 27:  // ESC
                                if (client->chat_input_mode) {
                                    // 채팅 입력 모드가 활성화된 상태에서 ESC 키
                                    disable_chat_input();
                                } else {
                                    // 일반 상태에서 ESC 키 - 메인 화면으로 돌아가기
                                    LOG_INFO("User returned to main screen from game");
                                    client->current_screen = SCREEN_MAIN;
                                }
                                break;
                            case '\n':
                            case '\r':
                                if (client->chat_input_mode) {
                                    // 채팅 입력 모드에서 엔터 - 채팅 입력 함수에서 처리
                                    handle_chat_input(ch);
                                } else {
                                    // 일반 모드에서 엔터 - 채팅 입력 모드 활성화
                                    LOG_DEBUG("User activated chat input mode");
                                    enable_chat_input();
                                }
                                break;
                            default:
                                // 채팅 입력 모드에서는 모든 키를 채팅 입력으로 처리
                                if (client->chat_input_mode) {
                                    handle_chat_input(ch);
                                }
                                break;
                        }
                        break;
                }
            }
        }

        // 화면 업데이트 (매번 수행)
        pthread_mutex_lock(&screen_mutex);

        // 에러 다이얼로그가 활성화되어 있으면 화면 업데이트를 건너뜀
        if (!client->error_dialog_active) {
            switch (client->current_screen) {
                case SCREEN_MAIN:
                    draw_main_screen();
                    break;
                case SCREEN_MATCHING:
                    draw_matching_screen();
                    break;
                case SCREEN_GAME:
                    draw_game_screen();
                    break;
                default:
                    draw_main_screen();
                    break;
            }
        }

        pthread_mutex_unlock(&screen_mutex);
        refresh();
    }

    // 프로그램 종료 (도달하지 않음)
    cleanup_network();
    cleanup_ncurses();
    logger_cleanup();

    return 0;
}