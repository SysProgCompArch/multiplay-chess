#include "handlers.h"
#include "../client_state.h"
#include "../logger.h"
#include <stdio.h>
#include <string.h>

int handle_match_game_response(ServerMessage *msg)
{
    if (msg->msg_case != SERVER_MESSAGE__MSG_MATCH_GAME_RES)
    {
        LOG_ERROR("Invalid message type for match game response handler");
        return -1;
    }

    if (msg->match_game_res)
    {
        if (msg->match_game_res->success)
        {
            LOG_INFO("Match found! Game ID: %s, Assigned color: %s",
                     msg->match_game_res->game_id ? msg->match_game_res->game_id : "unknown",
                     msg->match_game_res->assigned_color == COLOR__COLOR_WHITE ? "WHITE" : "BLACK");

            add_chat_message_safe("System", "Match found!");

            // 클라이언트 상태 업데이트
            client_state_t *client = get_client_state();
            strcpy(client->opponent_name, "Opponent");
            strcpy(client->game_state.opponent_player, "Opponent");
            client->is_white = (msg->match_game_res->assigned_color == COLOR__COLOR_WHITE);
            client->game_state.local_team = client->is_white ? TEAM_WHITE : TEAM_BLACK;
            client->game_state.game_in_progress = true;
            client->current_screen = SCREEN_GAME;
        }
        else
        {
            LOG_WARN("Matchmaking failed: %s",
                     msg->match_game_res->message ? msg->match_game_res->message : "no reason provided");

            add_chat_message_safe("System", "Matchmaking failed");
            client_state_t *client = get_client_state();
            client->current_screen = SCREEN_MAIN;
        }
    }
    else
    {
        LOG_ERROR("Received match game response but no response data");
    }

    return 0;
}