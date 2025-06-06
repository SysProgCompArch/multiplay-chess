#define _XOPEN_SOURCE_EXTENDED
#include <locale.h>
#include <ncursesw/ncurses.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
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

// 사용법 출력
void print_usage(const char *program_name) {
    printf("사용법: %s [OPTIONS]\n", program_name);
    printf("\n옵션:\n");
    printf("  -h, --host HOST    서버 호스트 (기본값: %s)\n", SERVER_DEFAULT_HOST);
    printf("  -p, --port PORT    서버 포트 (기본값: %d)\n", SERVER_DEFAULT_PORT);
    printf("  --help             이 도움말 출력\n");
    printf("\n예시:\n");
    printf("  %s                     # 기본값으로 연결\n", program_name);
    printf("  %s -h 192.168.1.100    # 특정 호스트로 연결\n", program_name);
    printf("  %s -p 9090             # 특정 포트로 연결\n", program_name);
    printf("  %s -h 192.168.1.100 -p 9090  # 호스트와 포트 모두 지정\n", program_name);
}

// 메인 함수
int main(int argc, char *argv[]) {
    // 기본값으로 초기화
    char server_host[256];
    int  server_port = SERVER_DEFAULT_PORT;
    strcpy(server_host, SERVER_DEFAULT_HOST);

    // 명령행 인자 처리
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--host") == 0) {
            if (i + 1 < argc) {
                strncpy(server_host, argv[i + 1], sizeof(server_host) - 1);
                server_host[sizeof(server_host) - 1] = '\0';
                i++;  // 다음 인자 건너뛰기
            } else {
                fprintf(stderr, "오류: %s 옵션에 호스트가 필요합니다.\n", argv[i]);
                print_usage(argv[0]);
                return 1;
            }
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
            if (i + 1 < argc) {
                server_port = atoi(argv[i + 1]);
                if (server_port <= 0 || server_port > 65535) {
                    fprintf(stderr, "오류: 잘못된 포트 번호: %s\n", argv[i + 1]);
                    return 1;
                }
                i++;  // 다음 인자 건너뛰기
            } else {
                fprintf(stderr, "오류: %s 옵션에 포트 번호가 필요합니다.\n", argv[i]);
                print_usage(argv[0]);
                return 1;
            }
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "오류: 알 수 없는 옵션: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    // 로거 초기화 (ncurses 초기화 전에) - 파일 출력 모드
    if (logger_init(LOG_OUTPUT_FILE, "chess_client") != 0) {
        fprintf(stderr, "Failed to initialize logger\n");
        return 1;
    }

    LOG_INFO("Chess client starting... (PID: %d)", getpid());
    LOG_INFO("Target server: %s:%d", server_host, server_port);

    // 신호 처리기 등록
    signal(SIGINT, handle_sigint);
    signal(SIGWINCH, handle_sigwinch);
    LOG_DEBUG("Signal handlers registered (SIGINT, SIGWINCH)");

    // 클라이언트 상태 초기화
    init_client_state();

    // 명령행에서 받은 서버 정보로 설정
    client_state_t *client = get_client_state();
    strcpy(client->server_host, server_host);
    client->server_port = server_port;

    LOG_DEBUG("Client state initialized with server %s:%d", client->server_host, client->server_port);

    // ncurses 초기화
    init_ncurses();
    LOG_INFO("ncurses initialized");

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
                } else if (client->game_state.opponent_disconnected) {
                    client->game_state.opponent_disconnected = false;
                    client->current_screen                   = SCREEN_MAIN;
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
                        // 채팅 입력 모드가 아닌 경우 키보드 조작 시도
                        if (!client->chat_input_mode && handle_keyboard_board_input(ch)) {
                            // 키보드 조작이 처리됨
                            break;
                        }

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
                            case 'q':
                            case 'Q':
                                // 메인 화면으로 돌아가기
                                LOG_INFO("User returned to main screen from game");
                                client->current_screen = SCREEN_MAIN;
                                break;
                            case 27:  // ESC
                                if (client->chat_input_mode) {
                                    // 채팅 입력 모드가 활성화된 상태에서 ESC 키
                                    disable_chat_input();
                                }
                                break;
                            case '\n':
                            case '\r':
                                if (client->chat_input_mode) {
                                    // 채팅 입력 모드에서 엔터 - 채팅 입력 함수에서 처리
                                    handle_chat_input(ch);
                                } else if (is_cursor_mode()) {
                                    // 키보드 조작에서 이미 엔터 처리됨
                                    break;
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
        } else if (client->game_state.opponent_disconnected && !client->dialog_active) {
            client->dialog_active = true;
            LOG_INFO("Showing opponent disconnected dialog");

            // 상대방 연결 끊김 다이얼로그 표시 (키 입력 처리 없음)
            draw_dialog("Opponent Disconnected", client->game_state.opponent_disconnect_message, "OK");
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