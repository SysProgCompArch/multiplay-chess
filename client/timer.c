// 맨 위에 아래 헤더들을 추가하세요.
#include "timer.h"           // update_player_timers(), send_timeout_message() 프로토타입
#include "client_state.h"    // client_state_t, get_client_state()
#include "game_state.h"      // game_state_t
#include "ui/ui.h"              // SCREEN_GAME, replay_mode 정의
#include "client_network.h"  // send_client_message()
#include "message.pb-c.h"    // ClientMessage, CLIENT_MESSAGE__INIT
 

// 서버로 시간초과를 알리는 메시지 전송
void send_timeout_message(void) {
    ClientMessage msg = CLIENT_MESSAGE__INIT;
    msg.msg_case = CLIENT_MESSAGE__MSG_TIMEOUT;    // 프로토콜에 추가된 타입
    send_client_message(&msg);
}

void update_player_timers() {
    client_state_t *cli = get_client_state();
    // 게임 화면이면서 리플레이 모드가 아닐 때만
    if (cli->current_screen == SCREEN_GAME && !cli->replay_mode) {
        game_state_t *gs = &cli->game_state;
        // 현재 턴인 쪽 시간만 감소
        if (gs->game.side_to_move == TEAM_WHITE) {
            if (gs->white_time_remaining > 0) gs->white_time_remaining--;
            if (gs->white_time_remaining == 0) {
                // 시간 초과 처리: 서버에 타임아웃 메시지 전송 또는 로컬 종료 처리
                send_timeout_message();
            }
        } else {
            if (gs->black_time_remaining > 0) gs->black_time_remaining--;
            if (gs->black_time_remaining == 0) {
                send_timeout_message();
            }
        }
    }
}
