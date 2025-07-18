#include "match_manager.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>  // gettimeofday를 위해 추가
#include <unistd.h>

#include "config.h"
#include "logger.h"
#include "network.h"
#include "utils.h"  // 체스판 초기화를 위해 추가

// 타이머 체크 스레드 관련 변수
bool      timer_thread_running = false;
pthread_t timer_thread_id      = 0;

// 매칭 매니저 전역 인스턴스
MatchManager g_match_manager;

// 밀리초 단위 현재 시간 가져오기
int64_t get_current_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

// 매칭 매니저 초기화
int init_match_manager(void) {
    memset(&g_match_manager, 0, sizeof(MatchManager));

    if (pthread_mutex_init(&g_match_manager.mutex, NULL) != 0) {
        log_perror("pthread_mutex_init");
        return -1;
    }

    g_match_manager.waiting_count     = 0;
    g_match_manager.active_game_count = 0;

    // 타이머 체크 스레드 시작
    if (start_timer_thread() != 0) {
        LOG_ERROR("Failed to start timer thread");
        pthread_mutex_destroy(&g_match_manager.mutex);
        return -1;
    }

    LOG_INFO("Match manager initialized");
    return 0;
}

// 매칭 매니저 정리
void cleanup_match_manager(void) {
    // 타이머 스레드 먼저 종료
    stop_timer_thread();

    pthread_mutex_lock(&g_match_manager.mutex);

    // 대기 중인 플레이어들 정리
    memset(g_match_manager.waiting_players, 0, sizeof(g_match_manager.waiting_players));
    g_match_manager.waiting_count = 0;

    // 활성 게임들 정리
    memset(g_match_manager.active_games, 0, sizeof(g_match_manager.active_games));
    g_match_manager.active_game_count = 0;

    pthread_mutex_unlock(&g_match_manager.mutex);
    pthread_mutex_destroy(&g_match_manager.mutex);

    LOG_INFO("Match manager cleaned up");
}

// 타이머 체크 스레드 시작
int start_timer_thread(void) {
    timer_thread_running = true;

    if (pthread_create(&timer_thread_id, NULL, timer_check_thread, NULL) != 0) {
        LOG_ERROR("Failed to create timer thread");
        timer_thread_running = false;
        return -1;
    }

    LOG_INFO("Timer check thread started");
    return 0;
}

// 타이머 체크 스레드 종료
void stop_timer_thread(void) {
    if (timer_thread_running) {
        timer_thread_running = false;

        if (timer_thread_id != 0) {
            pthread_join(timer_thread_id, NULL);
            timer_thread_id = 0;
            LOG_INFO("Timer check thread stopped");
        }
    }
}

// 타이머 체크 스레드 메인 함수 (100ms 간격으로 정밀하게 체크)
void *timer_check_thread(void *arg) {
    LOG_INFO("Timer check thread started");

    while (timer_thread_running) {
        check_game_timeouts();

        // 100ms마다 체크 (더 정밀한 타이머)
        usleep(100000);  // 100ms = 100,000 microseconds
    }

    LOG_INFO("Timer check thread terminated");
    return NULL;
}

// 게임 타이머 체크 및 시간 초과 처리 (밀리초 단위로 정밀도 향상)
void check_game_timeouts(void) {
    int64_t current_time_ms = get_current_time_ms();

    pthread_mutex_lock(&g_match_manager.mutex);

    for (int i = 0; i < MAX_ACTIVE_GAMES; i++) {
        ActiveGame *game = &g_match_manager.active_games[i];

        if (!game->is_active) {
            continue;
        }

        // 마지막 타이머 체크 이후 경과 시간만 차감 (중복 차감 방지)
        int64_t time_since_last_check_ms = current_time_ms - game->last_timer_check_ms;

        // 100ms 미만이면 건너뜀 (너무 빈번한 업데이트 방지)
        if (time_since_last_check_ms < 100) {
            continue;
        }

        team_t current_player = game->game_state.side_to_move;

        bool        timeout_occurred  = false;
        Team        timeout_winner    = TEAM__TEAM_UNSPECIFIED;
        const char *timeout_player_id = NULL;

        // 현재 턴인 플레이어의 시간만 차감 (밀리초 단위)
        if (current_player == TEAM_WHITE) {
            // 백 팀 턴인 경우
            game->white_time_remaining -= (int32_t)time_since_last_check_ms;
            if (game->white_time_remaining <= 0) {
                LOG_INFO("White player timeout in game %s (elapsed_ms: %ld, remaining was: %d ms)",
                         game->game_id, time_since_last_check_ms, game->white_time_remaining + (int32_t)time_since_last_check_ms);
                timeout_occurred           = true;
                timeout_winner             = TEAM__TEAM_BLACK;
                timeout_player_id          = game->white_player_id;
                game->white_time_remaining = 0;
            }
        } else {
            // 흑 팀 턴인 경우
            game->black_time_remaining -= (int32_t)time_since_last_check_ms;
            if (game->black_time_remaining <= 0) {
                LOG_INFO("Black player timeout in game %s (elapsed_ms: %ld, remaining was: %d ms)",
                         game->game_id, time_since_last_check_ms, game->black_time_remaining + (int32_t)time_since_last_check_ms);
                timeout_occurred           = true;
                timeout_winner             = TEAM__TEAM_WHITE;
                timeout_player_id          = game->black_player_id;
                game->black_time_remaining = 0;
            }
        }

        // 마지막 체크 시간 업데이트
        game->last_timer_check_ms = current_time_ms;

        if (timeout_occurred) {
            LOG_INFO("Game %s ended by timeout - winner: %s",
                     game->game_id,
                     (timeout_winner == TEAM__TEAM_WHITE) ? "WHITE" : "BLACK");

            // 게임 종료 브로드캐스트 전송
            send_timeout_game_end_broadcast(game, timeout_player_id, timeout_winner);

            // 게임 제거
            game->is_active = false;
            g_match_manager.active_game_count--;
        }
    }

    pthread_mutex_unlock(&g_match_manager.mutex);
}

// 시간 초과로 인한 게임 종료 브로드캐스트 전송
int send_timeout_game_end_broadcast(ActiveGame *game, const char *timeout_player_id, Team winner_team) {
    ServerMessage    game_end_msg       = SERVER_MESSAGE__INIT;
    GameEndBroadcast game_end_broadcast = GAME_END_BROADCAST__INIT;

    game_end_broadcast.game_id     = game->game_id;
    game_end_broadcast.player_id   = (char *)timeout_player_id;
    game_end_broadcast.winner_team = winner_team;
    game_end_broadcast.end_type    = GAME_END_TYPE__GAME_END_TIMEOUT;

    game_end_msg.msg_case = SERVER_MESSAGE__MSG_GAME_END;
    game_end_msg.game_end = &game_end_broadcast;

    // 양쪽 플레이어 모두에게 게임 종료 브로드캐스트 전송
    int result = 0;
    if (send_server_message(game->white_player_fd, &game_end_msg) < 0) {
        LOG_ERROR("Failed to send timeout game end broadcast to white player fd=%d", game->white_player_fd);
        result = -1;
    }
    if (send_server_message(game->black_player_fd, &game_end_msg) < 0) {
        LOG_ERROR("Failed to send timeout game end broadcast to black player fd=%d", game->black_player_fd);
        result = -1;
    }

    LOG_INFO("Sent timeout game end broadcast for game %s (timeout_player=%s, winner_team=%d)",
             game->game_id, timeout_player_id, winner_team);

    return result;
}

// 게임 ID 생성
char *generate_game_id(void) {
    static char game_id[GAME_ID_LENGTH + 1];
    static int  counter = 1;

    // 간단한 게임 ID 생성 (타임스탬프 + 카운터)
    snprintf(game_id, sizeof(game_id), "game_%ld_%d", time(NULL) % 100000, counter++);

    LOG_DEBUG("Generated game ID: %s", game_id);
    return game_id;
}

// 플레이어를 매칭에 추가
MatchResult add_player_to_matching(int fd, const char *player_id) {
    MatchResult result = {MATCH_STATUS_ERROR, NULL, TEAM__TEAM_UNSPECIFIED, -1, NULL, "Unknown error"};

    if (!player_id) {
        result.error_message = "Player ID is null";
        LOG_ERROR("add_player_to_matching: Player ID is null for fd=%d", fd);
        return result;
    }

    pthread_mutex_lock(&g_match_manager.mutex);

    // 이미 대기 중인 플레이어가 있는지 확인
    for (int i = 0; i < MAX_WAITING_PLAYERS; i++) {
        WaitingPlayer *waiting_player = &g_match_manager.waiting_players[i];
        if (waiting_player->is_active) {
            LOG_DEBUG("Found waiting player %s (fd=%d), attempting to create game",
                      waiting_player->player_id, waiting_player->fd);

            // 빈 게임 슬롯 찾기
            for (int j = 0; j < MAX_ACTIVE_GAMES; j++) {
                if (!g_match_manager.active_games[j].is_active) {
                    ActiveGame *game = &g_match_manager.active_games[j];

                    // 색상 랜덤 배정 (간단하게 시간 기반)
                    bool current_is_white = (time(NULL) % 2 == 0);

                    // 게임 정보 설정
                    strcpy(game->game_id, generate_game_id());
                    game->game_start_time = time(NULL);
                    game->is_active       = true;

                    // 체스판 초기화 (표준 시작 위치)
                    init_startpos(&game->game_state);

                    // 타이머 설정 (밀리초 단위)
                    game->time_limit_per_player = DEFAULT_GAME_TIME_LIMIT * 1000;  // 초를 밀리초로 변환
                    game->white_time_remaining  = DEFAULT_GAME_TIME_LIMIT * 1000;  // 초를 밀리초로 변환
                    game->black_time_remaining  = DEFAULT_GAME_TIME_LIMIT * 1000;  // 초를 밀리초로 변환
                    game->last_move_time_ms     = get_current_time_ms();
                    game->last_timer_check_ms   = get_current_time_ms();

                    if (current_is_white) {
                        game->white_player_fd = fd;
                        game->black_player_fd = waiting_player->fd;
                        strcpy(game->white_player_id, player_id);
                        strcpy(game->black_player_id, waiting_player->player_id);
                        result.assigned_team = TEAM__TEAM_WHITE;
                        result.opponent_name = waiting_player->player_id;
                    } else {
                        game->white_player_fd = waiting_player->fd;
                        game->black_player_fd = fd;
                        strcpy(game->white_player_id, waiting_player->player_id);
                        strcpy(game->black_player_id, player_id);
                        result.assigned_team = TEAM__TEAM_BLACK;
                        result.opponent_name = game->white_player_id;
                    }

                    g_match_manager.active_game_count++;

                    // 대기 목록에서 플레이어 제거
                    waiting_player->is_active = false;
                    g_match_manager.waiting_count--;

                    // 결과 설정
                    result.status        = MATCH_STATUS_GAME_STARTED;
                    result.game_id       = game->game_id;
                    result.opponent_fd   = (result.assigned_team == TEAM__TEAM_WHITE) ? game->black_player_fd : game->white_player_fd;
                    result.error_message = NULL;

                    LOG_INFO("Match found! Game %s: %s(fd=%d) vs %s(fd=%d)",
                             game->game_id,
                             game->white_player_id, game->white_player_fd,
                             game->black_player_id, game->black_player_fd);

                    pthread_mutex_unlock(&g_match_manager.mutex);
                    return result;
                }
            }

            // 게임 슬롯이 부족함
            result.error_message = "No available game slots";
            LOG_WARN("No available game slots for matching");
            pthread_mutex_unlock(&g_match_manager.mutex);
            return result;
        }
    }

    // 대기 중인 플레이어가 없음 - 대기 목록에 추가
    for (int i = 0; i < MAX_WAITING_PLAYERS; i++) {
        if (!g_match_manager.waiting_players[i].is_active) {
            WaitingPlayer *player = &g_match_manager.waiting_players[i];
            player->fd            = fd;
            strcpy(player->player_id, player_id);
            player->wait_start_time = time(NULL);
            player->is_active       = true;
            g_match_manager.waiting_count++;

            result.status        = MATCH_STATUS_WAITING;
            result.game_id       = "";
            result.assigned_team = TEAM__TEAM_UNSPECIFIED;
            result.opponent_name = NULL;
            result.error_message = NULL;

            LOG_INFO("Player %s(fd=%d) added to waiting queue", player_id, fd);

            pthread_mutex_unlock(&g_match_manager.mutex);
            return result;
        }
    }

    result.error_message = "Matching queue is full";
    LOG_WARN("Matching queue is full, cannot add player %s(fd=%d)", player_id, fd);
    pthread_mutex_unlock(&g_match_manager.mutex);
    return result;
}

// 플레이어를 매칭에서 제거
int remove_player_from_matching(int fd) {
    pthread_mutex_lock(&g_match_manager.mutex);

    // 대기 목록에서 제거
    for (int i = 0; i < MAX_WAITING_PLAYERS; i++) {
        if (g_match_manager.waiting_players[i].is_active &&
            g_match_manager.waiting_players[i].fd == fd) {
            g_match_manager.waiting_players[i].is_active = false;
            g_match_manager.waiting_count--;
            LOG_INFO("Player removed from waiting queue (fd=%d)", fd);
            pthread_mutex_unlock(&g_match_manager.mutex);
            return 0;
        }
    }

    LOG_DEBUG("Player not found in waiting queue (fd=%d)", fd);
    pthread_mutex_unlock(&g_match_manager.mutex);
    return -1;  // 플레이어를 찾지 못함
}

// 플레이어가 참여 중인 게임 찾기
ActiveGame *find_game_by_player_fd(int fd) {
    pthread_mutex_lock(&g_match_manager.mutex);

    for (int i = 0; i < MAX_ACTIVE_GAMES; i++) {
        if (g_match_manager.active_games[i].is_active &&
            (g_match_manager.active_games[i].white_player_fd == fd ||
             g_match_manager.active_games[i].black_player_fd == fd)) {
            pthread_mutex_unlock(&g_match_manager.mutex);
            return &g_match_manager.active_games[i];
        }
    }

    pthread_mutex_unlock(&g_match_manager.mutex);
    return NULL;
}

// 게임 제거
int remove_game(const char *game_id) {
    if (!game_id)
        return -1;

    pthread_mutex_lock(&g_match_manager.mutex);

    for (int i = 0; i < MAX_ACTIVE_GAMES; i++) {
        if (g_match_manager.active_games[i].is_active &&
            strcmp(g_match_manager.active_games[i].game_id, game_id) == 0) {
            g_match_manager.active_games[i].is_active = false;
            g_match_manager.active_game_count--;
            LOG_INFO("Game %s removed", game_id);
            pthread_mutex_unlock(&g_match_manager.mutex);
            return 0;
        }
    }

    LOG_DEBUG("Game %s not found for removal", game_id);
    pthread_mutex_unlock(&g_match_manager.mutex);
    return -1;  // 게임을 찾지 못함
}

// 매칭 매니저 상태 출력 (디버깅용)
void print_match_manager_status(void) {
    pthread_mutex_lock(&g_match_manager.mutex);

    LOG_INFO("=== Match Manager Status ===");
    LOG_INFO("Waiting players: %d/%d", g_match_manager.waiting_count, MAX_WAITING_PLAYERS);
    LOG_INFO("Active games: %d/%d", g_match_manager.active_game_count, MAX_ACTIVE_GAMES);

    LOG_DEBUG("Waiting players:");
    for (int i = 0; i < MAX_WAITING_PLAYERS; i++) {
        if (g_match_manager.waiting_players[i].is_active) {
            WaitingPlayer *p = &g_match_manager.waiting_players[i];
            LOG_DEBUG("  - %s (fd=%d, waiting for %ld seconds)",
                      p->player_id, p->fd, time(NULL) - p->wait_start_time);
        }
    }

    LOG_DEBUG("Active games:");
    for (int i = 0; i < MAX_ACTIVE_GAMES; i++) {
        if (g_match_manager.active_games[i].is_active) {
            ActiveGame *g = &g_match_manager.active_games[i];
            LOG_DEBUG("  - %s: %s(fd=%d) vs %s(fd=%d), running for %ld seconds",
                      g->game_id, g->white_player_id, g->white_player_fd,
                      g->black_player_id, g->black_player_fd,
                      time(NULL) - g->game_start_time);
        }
    }

    pthread_mutex_unlock(&g_match_manager.mutex);
}

// 대기 중인 플레이어 수 반환
int get_waiting_players_count(void) {
    pthread_mutex_lock(&g_match_manager.mutex);
    int count = g_match_manager.waiting_count;
    pthread_mutex_unlock(&g_match_manager.mutex);
    return count;
}

// 활성 게임 수 반환
int get_active_games_count(void) {
    pthread_mutex_lock(&g_match_manager.mutex);
    int count = g_match_manager.active_game_count;
    pthread_mutex_unlock(&g_match_manager.mutex);
    return count;
}

// 플레이어 연결 끊김 처리
int handle_player_disconnect(int fd) {
    pthread_mutex_lock(&g_match_manager.mutex);

    // 1. 대기 중인 플레이어인지 확인하고 제거
    for (int i = 0; i < MAX_WAITING_PLAYERS; i++) {
        if (g_match_manager.waiting_players[i].is_active &&
            g_match_manager.waiting_players[i].fd == fd) {
            g_match_manager.waiting_players[i].is_active = false;
            g_match_manager.waiting_count--;
            LOG_INFO("Disconnected player removed from waiting queue (fd=%d)", fd);
            pthread_mutex_unlock(&g_match_manager.mutex);
            return 0;
        }
    }

    // 2. 활성 게임에 참여 중인 플레이어인지 확인
    for (int i = 0; i < MAX_ACTIVE_GAMES; i++) {
        ActiveGame *game = &g_match_manager.active_games[i];
        if (game->is_active &&
            (game->white_player_fd == fd || game->black_player_fd == fd)) {
            // 상대방 fd 찾기
            int  opponent_fd;
            char disconnected_player_id[64];
            Team winner_team;

            if (game->white_player_fd == fd) {
                opponent_fd = game->black_player_fd;
                strcpy(disconnected_player_id, game->white_player_id);
                winner_team = TEAM__TEAM_BLACK;
            } else {
                opponent_fd = game->white_player_fd;
                strcpy(disconnected_player_id, game->black_player_id);
                winner_team = TEAM__TEAM_WHITE;
            }

            LOG_INFO("Player %s (fd=%d) disconnected from game %s, opponent is fd=%d",
                     disconnected_player_id, fd, game->game_id, opponent_fd);

            // 상대방에게 게임 종료 메시지 전송
            ServerMessage    disconnect_msg     = SERVER_MESSAGE__INIT;
            GameEndBroadcast game_end_broadcast = GAME_END_BROADCAST__INIT;

            game_end_broadcast.game_id     = game->game_id;
            game_end_broadcast.player_id   = disconnected_player_id;
            game_end_broadcast.winner_team = winner_team;
            game_end_broadcast.end_type    = GAME_END_TYPE__GAME_END_DISCONNECT;
            // timestamp는 NULL로 두면 protobuf에서 자동으로 처리

            disconnect_msg.msg_case = SERVER_MESSAGE__MSG_GAME_END;
            disconnect_msg.game_end = &game_end_broadcast;

            // 상대방에게 메시지 전송
            if (send_server_message(opponent_fd, &disconnect_msg) < 0) {
                LOG_WARN("Failed to send disconnect notification to opponent (fd=%d)", opponent_fd);
            } else {
                LOG_INFO("Sent disconnect notification to opponent (fd=%d)", opponent_fd);
            }

            // 게임 종료
            game->is_active = false;
            g_match_manager.active_game_count--;
            LOG_INFO("Game %s ended due to player disconnect", game->game_id);

            pthread_mutex_unlock(&g_match_manager.mutex);
            return 1;  // 게임에서 연결 끊김 처리됨
        }
    }

    LOG_DEBUG("Disconnected player (fd=%d) was not in any active game or waiting queue", fd);
    pthread_mutex_unlock(&g_match_manager.mutex);
    return -1;  // 매칭 상태가 아님
}