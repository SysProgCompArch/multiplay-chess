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
#include "game_save.h"
#include "logger.h"
#include "ui/ui.h"

// Function declarations
void replay_menu(void);

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
    // ungetch(KEY_RESIZE) 제거 - SA_RESTART가 제거되었으므로 getch()가 자동으로 중단됨

    // 데드락 방지를 위해 mutex를 시도해보고 실패시 강제 종료
    if (pthread_mutex_trylock(&screen_mutex) == 0) {
        // mutex를 성공적으로 획득했으면 unlock
        pthread_mutex_unlock(&screen_mutex);
    } else {
        // mutex 획득 실패 (다른 스레드가 잡고 있음) - 강제 종료
        LOG_WARN("Failed to acquire screen_mutex in signal handler, forcing exit");
        _exit(1);  // 즉시 종료 (cleanup 건너뛰기)
    }
}

void handle_sigtstp(int sig) {
    LOG_INFO("SIGTSTP received, cleaning up before suspension");
    // ncurses 정리 후 프로세스 일시정지
    cleanup_ncurses();

    // 기본 SIGTSTP 핸들러로 복원
    signal(SIGTSTP, SIG_DFL);
    raise(SIGTSTP);
}

void handle_sigcont(int sig) {
    LOG_INFO("SIGCONT received, resuming");
    // ncurses 재초기화
    init_ncurses();
    // SIGTSTP 핸들러 다시 등록
    signal(SIGTSTP, handle_sigtstp);
    // 화면 강제 새로고침
    terminal_resized = 1;
}

void handle_sigwinch(int sig) {
    terminal_resized = 1;
    LOG_DEBUG("Terminal resize signal received");
}

// 시그널 핸들러 설정 함수
void setup_signal_handlers() {
    struct sigaction sa;

    // SIGINT 핸들러 설정 (SA_RESTART 플래그 제거)
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    // sa.sa_flags = SA_RESTART;  // SA_RESTART 플래그 제거
    sa.sa_flags = 0;  // 시그널 발생 시 시스템 콜 중단
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        LOG_ERROR("Failed to set SIGINT handler");
        exit(1);
    }

    // SIGTSTP 핸들러 설정 (Ctrl+Z)
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigtstp;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;  // SA_RESTART 제거
    if (sigaction(SIGTSTP, &sa, NULL) == -1) {
        LOG_ERROR("Failed to set SIGTSTP handler");
        exit(1);
    }

    // SIGCONT 핸들러 설정 (프로세스 재개)
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigcont;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;  // SA_RESTART 제거
    if (sigaction(SIGCONT, &sa, NULL) == -1) {
        LOG_ERROR("Failed to set SIGCONT handler");
        exit(1);
    }

    // SIGWINCH 핸들러 설정 (터미널 크기 변경)
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigwinch;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;  // SA_RESTART 제거
    if (sigaction(SIGWINCH, &sa, NULL) == -1) {
        LOG_ERROR("Failed to set SIGWINCH handler");
        exit(1);
    }

    LOG_DEBUG("Signal handlers registered (SIGINT, SIGTSTP, SIGCONT, SIGWINCH) without SA_RESTART");
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

    // 시그널 핸들러 등록
    setup_signal_handlers();

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
    refresh();  // 초기 화면은 즉시 표시

    // 메인 루프 진입 전 강제 화면 업데이트 (초기 표시 보장)
    pthread_mutex_lock(&screen_mutex);
    draw_current_screen();
    pthread_mutex_unlock(&screen_mutex);
    refresh();

    LOG_INFO("Starting main game loop");

    // 메인 게임 루프
    // 이전 화면 상태를 추적하여 상태 변경 감지
    screen_state_t prev_screen = SCREEN_MAIN;

    while (1) {
        bool need_screen_update = false;  // 화면 업데이트 필요 여부 플래그

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
            need_screen_update = true;  // 리사이즈 후에는 화면 업데이트 필요
        }

        // 네트워크 스레드에 의한 화면 상태 변경 감지
        pthread_mutex_lock(&screen_mutex);
        screen_state_t current_screen           = client->current_screen;
        bool           network_update_requested = client->screen_update_requested;
        if (network_update_requested) {
            client->screen_update_requested = false;  // 플래그 리셋
            LOG_DEBUG("Network update flag detected and reset");
        }
        pthread_mutex_unlock(&screen_mutex);

        if (current_screen != prev_screen) {
            LOG_INFO("Screen state changed from %d to %d", prev_screen, current_screen);
            need_screen_update = true;
            prev_screen        = current_screen;

            // 게임 화면으로 전환 시 즉시 강제 업데이트
            if (current_screen == SCREEN_GAME) {
                LOG_INFO("Detected transition to game screen, forcing immediate update");
                pthread_mutex_lock(&screen_mutex);
                if (!client->dialog_active) {
                    draw_current_screen();
                }
                pthread_mutex_unlock(&screen_mutex);
                refresh();
            }
        }

        if (network_update_requested) {
            LOG_DEBUG("Network thread requested screen update");
            need_screen_update = true;
        }

        // 입력 처리 (논블로킹) - 시그널 처리를 위해 더 짧은 타임아웃 사용
        timeout(50);  // 0.05초 타임아웃 (시그널 반응성 향상)
        int ch = getch();

        // getch() 이후 즉시 시그널 상태 재확인
        if (shutdown_requested) {
            LOG_INFO("Shutdown requested detected after getch()");
            cleanup_and_exit(0);
        }

        // getch()가 시그널에 의해 중단된 경우 (ERR 반환)도 시그널 상태 확인
        if (ch == ERR && shutdown_requested) {
            LOG_INFO("getch() interrupted by signal, shutting down");
            cleanup_and_exit(0);
        }

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
                    prev_screen             = SCREEN_MAIN;  // 이전 상태도 업데이트
                    LOG_INFO("User acknowledged connection lost dialog, returned to main screen");
                } else if (client->game_state.opponent_disconnected) {
                    client->game_state.opponent_disconnected = false;
                    client->current_screen                   = SCREEN_MAIN;
                    prev_screen                              = SCREEN_MAIN;  // 이전 상태도 업데이트
                    LOG_INFO("User acknowledged opponent disconnected dialog, returned to main screen");
                } else {
                    // 기타 다이얼로그 (게임 종료, 기권 등)의 경우 메인 화면으로 돌아가기
                    client->current_screen = SCREEN_MAIN;
                    prev_screen            = SCREEN_MAIN;
                    LOG_INFO("User acknowledged dialog, returned to main screen");
                }

                draw_current_screen();
                pthread_mutex_unlock(&screen_mutex);

                refresh();  // 화면 즉시 갱신
            }
        } else if (ch != ERR && !dialog_active) {
            // LOG_DEBUG("Input received: %d", ch);
            need_screen_update = true;  // 사용자 입력이 있으면 화면 업데이트 필요

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
                                    prev_screen = SCREEN_MATCHING;  // 상태 변경 반영
                                }
                                break;
                            case '2':
                                LOG_INFO("User selected replay menu");
                                replay_menu();
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
                                prev_screen = SCREEN_MAIN;  // 상태 변경 반영
                                break;
                        }
                        break;

                    case SCREEN_GAME:
                        // 기권 다이얼로그가 활성화되어 있으면 다이얼로그 입력 처리
                        if (client->resign_dialog_active) {
                            handle_resign_dialog_input(ch);
                            break;
                        }

                        // 채팅 입력 모드가 아닌 경우 키보드 조작 시도
                        if (!client->chat_input_mode && handle_keyboard_board_input(ch)) {
                            // 키보드 조작이 처리됨
                            break;
                        }

                        switch (ch) {
                            case '1':
                                // 기권 다이얼로그 표시
                                if (!client->resign_dialog_active) {
                                    LOG_INFO("User requested resignation confirmation");
                                    show_resign_dialog();
                                }
                                break;
                            case '2':
                                // 무승부 제안
                                LOG_INFO("User offered a draw");
                                add_chat_message_safe(client->username, "offers a draw.");
                                break;
                            case '3':
                                // 게임 저장
                                // ▶ 자동 PGN 저장
                                LOG_INFO("Game — saving PGN...");
                                save_current_game(&client->game_state);
                                break;
                            case '4':
                            case 'q':
                            case 'Q':
                                // 메인 화면으로 돌아가기
                                LOG_INFO("User returned to main screen from game");
                                client->current_screen = SCREEN_MAIN;
                                prev_screen            = SCREEN_MAIN;  // 상태 변경 반영
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

        // 연결 끊김 상태 확인 및 다이얼로그 표시 (우선순위: 즉시 표시)
        pthread_mutex_lock(&screen_mutex);
        if (client->connection_lost && !client->dialog_active) {
            client->dialog_active = true;
            LOG_INFO("Showing connection lost dialog");

            // 연결 끊김 다이얼로그 표시 (키 입력 처리 없음)
            draw_dialog("Connection Lost", client->disconnect_message, "OK");
            pthread_mutex_unlock(&screen_mutex);
            refresh();  // 다이얼로그를 즉시 화면에 표시
            continue;   // 다른 처리를 건너뛰고 다시 루프 시작
        } else if (client->resign_result_dialog_pending && !client->dialog_active) {
            client->dialog_active                = true;
            client->resign_result_dialog_pending = false;
            LOG_INFO("Showing resign result dialog");

            // 기권 결과 다이얼로그 표시 (키 입력 처리 없음)
            draw_dialog(client->resign_result_title, client->resign_result_message, "OK");
            pthread_mutex_unlock(&screen_mutex);
            refresh();  // 다이얼로그를 즉시 화면에 표시
            continue;   // 다른 처리를 건너뛰고 다시 루프 시작
        } else if (client->game_state.opponent_disconnected && !client->dialog_active) {
            client->dialog_active = true;
            LOG_INFO("Showing opponent disconnected dialog");

            // 상대방 연결 끊김 다이얼로그 표시 (키 입력 처리 없음)
            draw_dialog("Opponent Disconnected", client->game_state.opponent_disconnect_message, "OK");
            pthread_mutex_unlock(&screen_mutex);
            refresh();  // 다이얼로그를 즉시 화면에 표시
            continue;   // 다른 처리를 건너뛰고 다시 루프 시작
        } else if (client->game_end_dialog_pending && !client->dialog_active) {
            client->dialog_active           = true;
            client->game_end_dialog_pending = false;
            LOG_INFO("Showing game end dialog");

            // 게임 종료 다이얼로그 표시 전에 먼저 화면 업데이트
            draw_current_screen();
            refresh();  // 화면 먼저 업데이트

            // 짧은 지연으로 사용자가 화면 변화를 볼 수 있도록 함
            usleep(100000);  // 100ms 지연

            // 게임 종료 다이얼로그 표시 (키 입력 처리 없음)
            draw_dialog("Game Over", client->game_end_message, "OK");
            pthread_mutex_unlock(&screen_mutex);
            refresh();  // 다이얼로그를 즉시 화면에 표시
            continue;   // 다른 처리를 건너뛰고 다시 루프 시작
        } else {
            pthread_mutex_unlock(&screen_mutex);
        }

        // 화면 업데이트 (필요한 경우에만 수행)
        // 게임 화면에서는 매칭 애니메이션이나 타이머 때문에 주기적 업데이트가 필요할 수 있음
        static time_t last_update_time       = 0;
        time_t        current_time           = time(NULL);
        bool          periodic_update_needed = ((current_screen == SCREEN_MATCHING || current_screen == SCREEN_GAME) &&
                                       current_time - last_update_time >= 1);  // 매칭 및 게임 화면에서는 1초마다 업데이트

        // 게임 화면에서 타이머 업데이트
        if (current_screen == SCREEN_GAME && periodic_update_needed) {
            pthread_mutex_lock(&screen_mutex);
            update_game_timer(&client->game_state);
            pthread_mutex_unlock(&screen_mutex);
        }

        // 네트워크 스레드에서 요청된 업데이트나 상태 변경이 있으면 즉시 업데이트
        if (need_screen_update || periodic_update_needed) {
            pthread_mutex_lock(&screen_mutex);

            // 에러 다이얼로그가 활성화되어 있으면 화면 업데이트를 건너뜀
            if (!client->dialog_active) {
                draw_current_screen();
            }

            pthread_mutex_unlock(&screen_mutex);
            refresh();

            if (periodic_update_needed) {
                last_update_time = current_time;
            }
        }

        // 네트워크 이벤트가 있을 때 즉시 반응하도록 추가 확인
        // 주로 게임 화면 전환을 위한 추가 보장
        if (!need_screen_update && !periodic_update_needed) {
            pthread_mutex_lock(&screen_mutex);
            if (client->screen_update_requested) {
                LOG_INFO("Additional network update detected, forcing screen refresh");
                client->screen_update_requested = false;

                // 다이얼로그가 활성화되어 있지 않을 때만 화면 업데이트
                if (!client->dialog_active) {
                    draw_current_screen();
                    pthread_mutex_unlock(&screen_mutex);
                    refresh();
                } else {
                    pthread_mutex_unlock(&screen_mutex);
                    // 다이얼로그가 활성화된 경우에는 다이얼로그 표시 로직에서 처리됨
                }
            } else {
                pthread_mutex_unlock(&screen_mutex);
            }
        }
    }

    // 프로그램 종료 (이 코드는 도달하지 않음)
    return 0;
}