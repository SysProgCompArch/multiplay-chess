#include "handlers.h"
#include "../client_state.h"
#include <stdio.h>
#include <string.h>

int handle_match_game_response(ServerMessage *msg)
{
    if (msg->msg_case != SERVER_MESSAGE__MSG_MATCH_GAME_RES)
    {
        return -1;
    }

    if (msg->match_game_res)
    {
        if (msg->match_game_res->success)
        {
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
            add_chat_message_safe("System", "Matchmaking failed");
            client_state_t *client = get_client_state();
            client->current_screen = SCREEN_MAIN;
        }
    }

    return 0;
}