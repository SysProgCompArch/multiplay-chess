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

// 종료 요청 플래그
volatile sig_atomic_t shutdown_requested = 0;

// 클린업 및 종료 함수
void cleanup_and_exit(int exit_code) {
    LOG_INFO("Starting cleanup and exit process");

    // 네트워크 정리
    cleanup_network();

    // UI 다이얼로그 정리
    close_dialog();

    // ncurses 정리
    cleanup_ncurses();

    // 로거 정리
    logger_cleanup();

    LOG_INFO("Cleanup completed, exiting with code %d", exit_code);
    exit(exit_code);
}

void handle_sigint(int sig) {
    shutdown_requested = 1;
    LOG_INFO("SIGINT received, requesting graceful shutdown");
}

void handle_sigwinch(int sig) {
    terminal_resized = 1;
    LOG_DEBUG("Terminal resize signal received");
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
    signal(SIGINT, handle_sigint);
    signal(SIGWINCH, handle_sigwinch);
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
        cleanup_and_exit(1);
    }
    LOG_INFO("Network thread started");

    // 첫 화면 그리기
    draw_main_screen();

    client_state_t *client = get_client_state();
    LOG_INFO("Starting main game loop");

    // 메인 게임 루프
    while (1) {
        // 종료 요청 확인 (SIGINT)
        if (shutdown_requested) {
            LOG_INFO("Shutdown requested, exiting gracefully");
            cleanup_and_exit(0);
        }

        // 터미널 크기 변경 확인
        if (terminal_resized) {
            terminal_resized = 0;
            LOG_INFO("Handling terminal resize");
            handle_terminal_resize();
        }

        // 입력 처리 (논블로킹)
        timeout(100);  // 0.1초 타임아웃 (반응성 향상)
        int ch = getch();

        // 에러 다이얼로그가 활성화되어 있는지 확인
        pthread_mutex_lock(&screen_mutex);
        bool dialog_active = client->dialog_active;
        pthread_mutex_unlock(&screen_mutex);

        if (ch != ERR && dialog_active) {
            // 다이얼로그가 활성화되어 있을 때 Enter 키 처리
            if (ch == '\n' || ch == '\r' || ch == ' ') {
                LOG_DEBUG("User acknowledged dialog with key: %d", ch);
                pthread_mutex_lock(&screen_mutex);

                // 다이얼로그 닫기
                close_dialog();
                client->dialog_active = false;

                // 연결 끊김 타입에 따라 상태 리셋
                if (client->connection_lost) {
                    client->connection_lost = false;
                    client->current_screen  = SCREEN_MAIN;
                    LOG_INFO("User acknowledged connection lost dialog, returned to main screen");
                } else if (client->opponent_disconnected) {
                    client->opponent_disconnected = false;
                    client->current_screen        = SCREEN_MAIN;
                    LOG_INFO("User acknowledged opponent disconnected dialog, returned to main screen");
                }

                pthread_mutex_unlock(&screen_mutex);
            }
        } else if (ch != ERR && !dialog_active) {
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
                                cleanup_and_exit(0);
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

        // 연결 끊김 상태 확인 및 다이얼로그 표시
        pthread_mutex_lock(&screen_mutex);
        if (client->connection_lost && !client->dialog_active) {
            client->dialog_active = true;
            LOG_INFO("Showing connection lost dialog");

            // 연결 끊김 다이얼로그 표시 (키 입력 처리 없음)
            draw_dialog("Connection Lost", client->disconnect_message, "OK");
            pthread_mutex_unlock(&screen_mutex);
        } else if (client->opponent_disconnected && !client->dialog_active) {
            client->dialog_active = true;
            LOG_INFO("Showing opponent disconnected dialog");

            // 상대방 연결 끊김 다이얼로그 표시 (키 입력 처리 없음)
            draw_dialog("Opponent Disconnected", client->opponent_disconnect_message, "OK");
            pthread_mutex_unlock(&screen_mutex);
        } else {
            pthread_mutex_unlock(&screen_mutex);
        }

        // 화면 업데이트 (매번 수행)
        pthread_mutex_lock(&screen_mutex);

        // 에러 다이얼로그가 활성화되어 있으면 화면 업데이트를 건너뜀
        if (!client->dialog_active) {
            draw_current_screen();
        }

        pthread_mutex_unlock(&screen_mutex);
        refresh();
    }

    // 프로그램 종료 (이 코드는 도달하지 않음)
    return 0;
}