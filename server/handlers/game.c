// 향후 체스 게임 관련 메시지 처리를 위한 예시 핸들러
// 이 파일은 아직 사용되지 않으며, 프로토콜에 게임 메시지가 추가되면 구현 예정

/*
#include "handlers.h"
#include <stdio.h>
#include <stdlib.h>

// 게임 참가 요청 처리 핸들러 (예시)
int handle_join_game_message(int fd, ClientMessage *req)
{
    if (req->msg_case != CLIENT_MESSAGE__MSG_JOIN_GAME)
    {
        fprintf(stderr, "[fd=%d] Invalid message type for join game handler\n", fd);
        return -1;
    }

    // TODO: 매치메이킹 로직 구현
    printf("[fd=%d] Player wants to join game\n", fd);

    // 임시 응답
    ServerMessage resp = SERVER_MESSAGE__INIT;
    // resp.msg_case = SERVER_MESSAGE__MSG_JOIN_GAME_RES;
    // resp.join_game_res = &join_game_response;

    return send_server_message(fd, &resp);
}

// 체스 수 이동 처리 핸들러 (예시)
int handle_move_message(int fd, ClientMessage *req)
{
    if (req->msg_case != CLIENT_MESSAGE__MSG_MOVE)
    {
        fprintf(stderr, "[fd=%d] Invalid message type for move handler\n", fd);
        return -1;
    }

    // TODO: 체스 규칙 검증 및 게임 상태 업데이트
    printf("[fd=%d] Player made a move: %s -> %s\n", fd,
           req->move->from, req->move->to);

    // TODO: 상대방에게 수 전달

    return 0;
}

// 채팅 메시지 처리 핸들러 (예시)
int handle_chat_message(int fd, ClientMessage *req)
{
    if (req->msg_case != CLIENT_MESSAGE__MSG_CHAT)
    {
        fprintf(stderr, "[fd=%d] Invalid message type for chat handler\n", fd);
        return -1;
    }

    // TODO: 게임 룸의 다른 플레이어에게 채팅 전달
    printf("[fd=%d] Chat: %s\n", fd, req->chat->message);

    return 0;
}
*/