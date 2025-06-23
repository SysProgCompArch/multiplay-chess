// server/handlers/timeout.c

#include "handlers.h"
#include "match_manager.h"                     // ActiveGame 정의 :contentReference[oaicite:0]{index=0}
#include "../server_network.h"                    // broadcast_game_end()
#include "../../common/generated/message.pb-c.h"  // GAME_END_TYPE__GAME_END_TIMEOUT
#include "../logger.h"                            // LOG_INFO, LOG_ERROR

int handle_timeout_message(int fd, ClientMessage *req) {
    ActiveGame *game = find_game_by_player_fd(fd);
    if (!game) {
        LOG_ERROR("Timeout from unknown fd=%d", fd);
        return -1;
    }

    const char *player = (fd == game->white_player_fd)
                         ? game->white_player_id
                         : game->black_player_id;
    LOG_INFO("Player %s timed out (fd=%d)", player, fd);

    broadcast_game_end(
        game,
        GAME_END_TYPE__GAME_END_TIMEOUT,
        "Time expired"
    );
    
    remove_game(game->game_id);
    return 0;
}
