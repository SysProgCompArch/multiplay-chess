#include "network.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "logger.h"

// 모든 데이터를 전송할 때까지 반복하여 전송한다
ssize_t send_all(int sockfd, const void *buf, size_t len) {
    size_t         total = 0;
    const uint8_t *p     = buf;
    while (total < len) {
        ssize_t sent = send(sockfd, p + total, len - total, 0);
        if (sent <= 0) {
            if (sent < 0 && errno == EINTR)
                continue;
            perror("send");
            return -1;
        }
        total += sent;
    }
    return total;
}

// 지정된 바이트 수만큼 수신할 때까지 반복하여 수신한다
ssize_t recv_all(int sockfd, void *buf, size_t len) {
    size_t   total = 0;
    uint8_t *p     = buf;
    while (total < len) {
        ssize_t recvd = recv(sockfd, p + total, len - total, 0);
        if (recvd < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        } else if (recvd == 0) {
            return 0;
        }
        total += recvd;
    }
    return total;
}

// ClientMessage를 직렬화해서 전송
int send_client_message(int fd, ClientMessage *msg) {
    size_t   plen = client_message__get_packed_size(msg);
    uint8_t *sb   = malloc(4 + plen);
    if (!sb) {
        perror("malloc");
        return -1;
    }

    uint32_t nl = htonl(plen);
    memcpy(sb, &nl, 4);
    client_message__pack(msg, sb + 4);

    LOG_DEBUG("Sending message to fd=%d, size=%zu bytes", fd, plen);
    int result = send_all(fd, sb, 4 + plen);
    free(sb);

    if (result < 0) {
        LOG_ERROR("Failed to send message to fd=%d", fd);
    } else {
        LOG_DEBUG("Message sent successfully to fd=%d", fd);
    }

    return result;
}

// ServerMessage를 직렬화해서 전송
int send_server_message(int fd, ServerMessage *msg) {
    size_t   plen = server_message__get_packed_size(msg);
    uint8_t *sb   = malloc(4 + plen);
    if (!sb) {
        log_perror("malloc");
        return -1;
    }

    uint32_t nl = htonl(plen);
    memcpy(sb, &nl, 4);
    server_message__pack(msg, sb + 4);

    LOG_DEBUG("Sending message to fd=%d, size=%zu bytes", fd, plen);
    int result = send_all(fd, sb, 4 + plen);
    free(sb);

    if (result < 0) {
        LOG_ERROR("Failed to send message to fd=%d", fd);
    } else {
        LOG_DEBUG("Message sent successfully to fd=%d", fd);
    }

    return result;
}

// 클라이언트로부터 메시지를 수신하고 파싱
ClientMessage *receive_client_message(int fd) {
    // length-prefix protobuf 메시지 수신
    uint8_t lenbuf[4];
    ssize_t r = recv_all(fd, lenbuf, 4);
    if (r <= 0) {
        if (r < 0) {
            LOG_DEBUG("Failed to receive message length from fd=%d", fd);
        }
        return NULL;  // 연결 종료 또는 에러
    }

    uint32_t msg_len;
    memcpy(&msg_len, lenbuf, 4);
    msg_len = ntohl(msg_len);

    LOG_DEBUG("Receiving message from fd=%d, expected size=%u bytes", fd, msg_len);

    uint8_t *buf = malloc(msg_len);
    if (!buf) {
        log_perror("malloc");
        return NULL;
    }

    r = recv_all(fd, buf, msg_len);
    if (r <= 0) {
        LOG_DEBUG("Failed to receive message body from fd=%d", fd);
        free(buf);
        return NULL;  // 연결 종료 또는 에러
    }

    // 메시지 역직렬화
    ClientMessage *msg = client_message__unpack(NULL, msg_len, buf);
    free(buf);

    if (msg) {
        LOG_DEBUG("Message received and parsed from fd=%d, msg_case=%d", fd, msg->msg_case);
    } else {
        LOG_WARN("Failed to parse message from fd=%d", fd);
    }

    return msg;
}

// 서버로부터 메시지를 수신하고 파싱
ServerMessage *receive_server_message(int fd) {
    // length-prefix protobuf 메시지 수신
    uint8_t lenbuf[4];
    ssize_t r = recv_all(fd, lenbuf, 4);
    if (r <= 0) {
        LOG_DEBUG("Failed to receive message length from fd=%d", fd);
        return NULL;  // 연결 종료 또는 에러
    }

    uint32_t msg_len;
    memcpy(&msg_len, lenbuf, 4);
    msg_len = ntohl(msg_len);

    LOG_DEBUG("Receiving message from fd=%d, expected size=%u bytes", fd, msg_len);

    if (msg_len > 1024 * 1024)  // 1MB 제한
    {
        return NULL;
    }

    uint8_t *buf = malloc(msg_len);
    if (!buf) {
        perror("malloc");
        return NULL;
    }

    r = recv_all(fd, buf, msg_len);
    if (r <= 0) {
        free(buf);
        return NULL;  // 연결 종료 또는 에러
    }

    // 메시지 역직렬화
    ServerMessage *msg = server_message__unpack(NULL, msg_len, buf);
    free(buf);

    if (msg) {
        LOG_DEBUG("Message received and parsed from fd=%d, msg_case=%d", fd, msg->msg_case);
    } else {
        LOG_WARN("Failed to parse message from fd=%d", fd);
    }

    return msg;
}