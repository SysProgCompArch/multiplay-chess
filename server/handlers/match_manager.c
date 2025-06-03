#include "match_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

// 매칭 매니저 전역 인스턴스
MatchManager g_match_manager;

// 매칭 매니저 초기화
int init_match_manager(void)
{
    memset(&g_match_manager, 0, sizeof(MatchManager));

    if (pthread_mutex_init(&g_match_manager.mutex, NULL) != 0)
    {
        perror("pthread_mutex_init");
        return -1;
    }

    g_match_manager.waiting_count = 0;
    g_match_manager.active_game_count = 0;

    printf("Match manager initialized\n");
    return 0;
}

// 매칭 매니저 정리
void cleanup_match_manager(void)
{
    pthread_mutex_lock(&g_match_manager.mutex);

    // 대기 중인 플레이어들 정리
    memset(g_match_manager.waiting_players, 0, sizeof(g_match_manager.waiting_players));
    g_match_manager.waiting_count = 0;

    // 활성 게임들 정리
    memset(g_match_manager.active_games, 0, sizeof(g_match_manager.active_games));
    g_match_manager.active_game_count = 0;

    pthread_mutex_unlock(&g_match_manager.mutex);
    pthread_mutex_destroy(&g_match_manager.mutex);

    printf("Match manager cleaned up\n");
}

// 게임 ID 생성
char *generate_game_id(void)
{
    static char game_id[GAME_ID_LENGTH + 1];
    static int counter = 1;

    // 간단한 게임 ID 생성 (타임스탬프 + 카운터)
    snprintf(game_id, sizeof(game_id), "game_%ld_%d", time(NULL) % 100000, counter++);

    return game_id;
}

// 플레이어를 매칭에 추가
MatchResult add_player_to_matching(int fd, const char *player_id)
{
    MatchResult result = {0};
    result.status = MATCH_STATUS_ERROR;
    result.error_message = "Unknown error";

    if (!player_id)
    {
        result.error_message = "Invalid player ID";
        return result;
    }

    pthread_mutex_lock(&g_match_manager.mutex);

    // 이미 대기 중인 플레이어인지 확인
    for (int i = 0; i < MAX_WAITING_PLAYERS; i++)
    {
        if (g_match_manager.waiting_players[i].is_active &&
            g_match_manager.waiting_players[i].fd == fd)
        {
            result.error_message = "Player already in matching queue";
            pthread_mutex_unlock(&g_match_manager.mutex);
            return result;
        }
    }

    // 이미 게임 중인 플레이어인지 확인
    for (int i = 0; i < MAX_ACTIVE_GAMES; i++)
    {
        if (g_match_manager.active_games[i].is_active &&
            (g_match_manager.active_games[i].white_player_fd == fd ||
             g_match_manager.active_games[i].black_player_fd == fd))
        {
            result.error_message = "Player already in a game";
            pthread_mutex_unlock(&g_match_manager.mutex);
            return result;
        }
    }

    // 대기 중인 다른 플레이어가 있는지 확인
    for (int i = 0; i < MAX_WAITING_PLAYERS; i++)
    {
        if (g_match_manager.waiting_players[i].is_active)
        {
            // 매칭 성공! 게임 시작
            WaitingPlayer *waiting_player = &g_match_manager.waiting_players[i];

            // 빈 게임 슬롯 찾기
            int game_slot = -1;
            for (int j = 0; j < MAX_ACTIVE_GAMES; j++)
            {
                if (!g_match_manager.active_games[j].is_active)
                {
                    game_slot = j;
                    break;
                }
            }

            if (game_slot == -1)
            {
                result.error_message = "No available game slots";
                pthread_mutex_unlock(&g_match_manager.mutex);
                return result;
            }

            // 새 게임 생성
            ActiveGame *game = &g_match_manager.active_games[game_slot];
            strcpy(game->game_id, generate_game_id());

            // 랜덤하게 색상 배정 (첫 번째 플레이어가 흰색일 확률 50%)
            if (rand() % 2 == 0)
            {
                // 대기 중인 플레이어가 흰색
                game->white_player_fd = waiting_player->fd;
                game->black_player_fd = fd;
                strcpy(game->white_player_id, waiting_player->player_id);
                strcpy(game->black_player_id, player_id);
                result.assigned_color = COLOR__COLOR_BLACK;
            }
            else
            {
                // 새 플레이어가 흰색
                game->white_player_fd = fd;
                game->black_player_fd = waiting_player->fd;
                strcpy(game->white_player_id, player_id);
                strcpy(game->black_player_id, waiting_player->player_id);
                result.assigned_color = COLOR__COLOR_WHITE;
            }

            game->game_start_time = time(NULL);
            game->is_active = true;
            g_match_manager.active_game_count++;

            // 대기 목록에서 플레이어 제거
            waiting_player->is_active = false;
            g_match_manager.waiting_count--;

            // 결과 설정
            result.status = MATCH_STATUS_GAME_STARTED;
            result.game_id = game->game_id;
            result.opponent_fd = (result.assigned_color == COLOR__COLOR_WHITE) ? game->black_player_fd : game->white_player_fd;
            result.error_message = NULL;

            printf("Match found! Game %s: %s(fd=%d) vs %s(fd=%d)\n",
                   game->game_id,
                   game->white_player_id, game->white_player_fd,
                   game->black_player_id, game->black_player_fd);

            pthread_mutex_unlock(&g_match_manager.mutex);
            return result;
        }
    }

    // 대기 중인 플레이어가 없음 - 대기 목록에 추가
    for (int i = 0; i < MAX_WAITING_PLAYERS; i++)
    {
        if (!g_match_manager.waiting_players[i].is_active)
        {
            WaitingPlayer *player = &g_match_manager.waiting_players[i];
            player->fd = fd;
            strcpy(player->player_id, player_id);
            player->wait_start_time = time(NULL);
            player->is_active = true;
            g_match_manager.waiting_count++;

            result.status = MATCH_STATUS_WAITING;
            result.game_id = "";
            result.assigned_color = COLOR__COLOR_UNSPECIFIED;
            result.error_message = NULL;

            printf("Player %s(fd=%d) added to waiting queue\n", player_id, fd);

            pthread_mutex_unlock(&g_match_manager.mutex);
            return result;
        }
    }

    result.error_message = "Matching queue is full";
    pthread_mutex_unlock(&g_match_manager.mutex);
    return result;
}

// 플레이어를 매칭에서 제거
int remove_player_from_matching(int fd)
{
    pthread_mutex_lock(&g_match_manager.mutex);

    // 대기 목록에서 제거
    for (int i = 0; i < MAX_WAITING_PLAYERS; i++)
    {
        if (g_match_manager.waiting_players[i].is_active &&
            g_match_manager.waiting_players[i].fd == fd)
        {
            g_match_manager.waiting_players[i].is_active = false;
            g_match_manager.waiting_count--;
            printf("Player removed from waiting queue (fd=%d)\n", fd);
            pthread_mutex_unlock(&g_match_manager.mutex);
            return 0;
        }
    }

    pthread_mutex_unlock(&g_match_manager.mutex);
    return -1; // 플레이어를 찾지 못함
}

// 플레이어가 참여 중인 게임 찾기
ActiveGame *find_game_by_player_fd(int fd)
{
    pthread_mutex_lock(&g_match_manager.mutex);

    for (int i = 0; i < MAX_ACTIVE_GAMES; i++)
    {
        if (g_match_manager.active_games[i].is_active &&
            (g_match_manager.active_games[i].white_player_fd == fd ||
             g_match_manager.active_games[i].black_player_fd == fd))
        {
            pthread_mutex_unlock(&g_match_manager.mutex);
            return &g_match_manager.active_games[i];
        }
    }

    pthread_mutex_unlock(&g_match_manager.mutex);
    return NULL;
}

// 게임 제거
int remove_game(const char *game_id)
{
    if (!game_id)
        return -1;

    pthread_mutex_lock(&g_match_manager.mutex);

    for (int i = 0; i < MAX_ACTIVE_GAMES; i++)
    {
        if (g_match_manager.active_games[i].is_active &&
            strcmp(g_match_manager.active_games[i].game_id, game_id) == 0)
        {
            g_match_manager.active_games[i].is_active = false;
            g_match_manager.active_game_count--;
            printf("Game %s removed\n", game_id);
            pthread_mutex_unlock(&g_match_manager.mutex);
            return 0;
        }
    }

    pthread_mutex_unlock(&g_match_manager.mutex);
    return -1; // 게임을 찾지 못함
}

// 매칭 매니저 상태 출력 (디버깅용)
void print_match_manager_status(void)
{
    pthread_mutex_lock(&g_match_manager.mutex);

    printf("=== Match Manager Status ===\n");
    printf("Waiting players: %d/%d\n", g_match_manager.waiting_count, MAX_WAITING_PLAYERS);
    printf("Active games: %d/%d\n", g_match_manager.active_game_count, MAX_ACTIVE_GAMES);

    printf("Waiting players:\n");
    for (int i = 0; i < MAX_WAITING_PLAYERS; i++)
    {
        if (g_match_manager.waiting_players[i].is_active)
        {
            WaitingPlayer *p = &g_match_manager.waiting_players[i];
            printf("  - %s (fd=%d, waiting for %ld seconds)\n",
                   p->player_id, p->fd, time(NULL) - p->wait_start_time);
        }
    }

    printf("Active games:\n");
    for (int i = 0; i < MAX_ACTIVE_GAMES; i++)
    {
        if (g_match_manager.active_games[i].is_active)
        {
            ActiveGame *g = &g_match_manager.active_games[i];
            printf("  - %s: %s(fd=%d) vs %s(fd=%d), running for %ld seconds\n",
                   g->game_id, g->white_player_id, g->white_player_fd,
                   g->black_player_id, g->black_player_fd,
                   time(NULL) - g->game_start_time);
        }
    }

    pthread_mutex_unlock(&g_match_manager.mutex);
}

// 대기 중인 플레이어 수 반환
int get_waiting_players_count(void)
{
    pthread_mutex_lock(&g_match_manager.mutex);
    int count = g_match_manager.waiting_count;
    pthread_mutex_unlock(&g_match_manager.mutex);
    return count;
}

// 활성 게임 수 반환
int get_active_games_count(void)
{
    pthread_mutex_lock(&g_match_manager.mutex);
    int count = g_match_manager.active_game_count;
    pthread_mutex_unlock(&g_match_manager.mutex);
    return count;
}