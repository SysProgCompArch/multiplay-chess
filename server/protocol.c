#include "protocol.h"
#include "handlers/handlers.h"
#include "handlers/match_manager.h"
#include "server_network.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

// 모든 데이터를 전송할 때까지 반복하여 전송한다
ssize_t send_all(int sockfd, const void *buf, size_t len)
{
    size_t total = 0;
    const uint8_t *p = buf;
    while (total < len)
    {
        ssize_t sent = send(sockfd, p + total, len - total, 0);
        if (sent <= 0)
        {
            if (sent < 0 && errno == EINTR)
                continue;
            log_perror("send");
            return -1;
        }
        total += sent;
    }
    return total;
}

// 지정된 바이트 수만큼 수신할 때까지 반복하여 수신한다
ssize_t recv_all(int sockfd, void *buf, size_t len)
{
    size_t total = 0;
    uint8_t *p = buf;
    while (total < len)
    {
        ssize_t recvd = recv(sockfd, p + total, len - total, 0);
        if (recvd < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        else if (recvd == 0)
        {
            return 0;
        }
        total += recvd;
    }
    return total;
}

// ServerMessage를 직렬화해서 클라이언트에게 전송
int send_server_message(int fd, ServerMessage *msg)
{
    size_t plen = server_message__get_packed_size(msg);
    uint8_t *sb = malloc(4 + plen);
    if (!sb)
    {
        log_perror("malloc");
        return -1;
    }

    uint32_t nl = htonl(plen);
    memcpy(sb, &nl, 4);
    server_message__pack(msg, sb + 4);

    LOG_DEBUG("Sending message to fd=%d, size=%zu bytes", fd, plen);
    int result = send_all(fd, sb, 4 + plen);
    free(sb);

    if (result < 0)
    {
        LOG_ERROR("Failed to send message to fd=%d", fd);
    }
    else
    {
        LOG_DEBUG("Message sent successfully to fd=%d", fd);
    }

    return result;
}

// 클라이언트로부터 메시지를 수신하고 파싱
ClientMessage *receive_client_message(int fd)
{
    // length-prefix protobuf 메시지 수신
    uint8_t lenbuf[4];
    ssize_t r = recv_all(fd, lenbuf, 4);
    if (r <= 0)
    {
        if (r < 0)
        {
            LOG_DEBUG("Failed to receive message length from fd=%d", fd);
        }
        return NULL; // 연결 종료 또는 에러
    }

    uint32_t msg_len;
    memcpy(&msg_len, lenbuf, 4);
    msg_len = ntohl(msg_len);

    LOG_DEBUG("Receiving message from fd=%d, expected size=%u bytes", fd, msg_len);

    uint8_t *buf = malloc(msg_len);
    if (!buf)
    {
        log_perror("malloc");
        return NULL;
    }

    r = recv_all(fd, buf, msg_len);
    if (r <= 0)
    {
        LOG_DEBUG("Failed to receive message body from fd=%d", fd);
        free(buf);
        return NULL; // 연결 종료 또는 에러
    }

    // 메시지 역직렬화
    ClientMessage *req = client_message__unpack(NULL, msg_len, buf);
    free(buf);

    if (req)
    {
        LOG_DEBUG("Message received and parsed from fd=%d, msg_case=%d", fd, req->msg_case);
    }
    else
    {
        LOG_WARN("Failed to parse message from fd=%d", fd);
    }

    return req;
}

// 클라이언트 소켓에서 protobuf 메시지 수신 및 처리, 연결 종료 처리
void handle_client_message(int fd, int epfd)
{
    ClientMessage *req = receive_client_message(fd);
    if (!req)
    {
        LOG_INFO("Client disconnected: fd=%d", fd);

        // 매칭 큐에서 플레이어 제거
        remove_player_from_matching(fd);

        // TODO: 진행 중인 게임이 있다면 상대방에게 알림
        ActiveGame *game = find_game_by_player_fd(fd);
        if (game)
        {
            LOG_INFO("Player disconnected from game %s: fd=%d", game->game_id, fd);
            // 상대방에게 연결 해제 알림 (나중에 구현)
            remove_game(game->game_id);
        }

        close(fd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
        return;
    }

    // 메시지 타입에 따라 적절한 핸들러로 디스패치
    int result = 0;
    switch (req->msg_case)
    {
    case CLIENT_MESSAGE__MSG_PING:
        LOG_DEBUG("Handling PING message from fd=%d", fd);
        result = handle_ping_message(fd, req);
        break;

    case CLIENT_MESSAGE__MSG_ECHO:
        LOG_DEBUG("Handling ECHO message from fd=%d", fd);
        result = handle_echo_message(fd, req);
        break;

    case CLIENT_MESSAGE__MSG_MATCH_GAME:
        LOG_DEBUG("Handling MATCH_GAME message from fd=%d", fd);
        result = handle_match_game_message(fd, req);
        break;

        // 나중에 추가될 메시지 타입들
        // case CLIENT_MESSAGE__MSG_MOVE:
        //     result = handle_move_message(fd, req);
        //     break;
        //
        // case CLIENT_MESSAGE__MSG_JOIN_GAME:
        //     result = handle_join_game_message(fd, req);
        //     break;

    default:
        LOG_WARN("Unsupported message type from fd=%d: msg_case=%d", fd, req->msg_case);

        // 에러 응답 생성 및 전송
        ServerMessage error_resp = SERVER_MESSAGE__INIT;
        ErrorResponse error = ERROR_RESPONSE__INIT;
        error.message = "Unsupported message type";
        error_resp.msg_case = SERVER_MESSAGE__MSG_ERROR;
        error_resp.error = &error;

        result = send_server_message(fd, &error_resp);
        if (result < 0)
        {
            LOG_ERROR("Failed to send error response to fd=%d, closing connection", fd);
        }
        else
        {
            LOG_DEBUG("Error response sent successfully to fd=%d", fd);
            result = 0; // 에러 응답을 성공적으로 보냈으므로 연결 유지
        }
        break;
    }

    // 핸들러에서 에러가 발생하면 연결을 닫을 수도 있음
    if (result < 0)
    {
        LOG_WARN("Handler error for fd=%d, closing connection", fd);
        close(fd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
    }

    client_message__free_unpacked(req, NULL);
}