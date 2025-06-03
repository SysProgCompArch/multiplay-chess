#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <sys/types.h>
#include "message.pb-c.h"

// 클라이언트 메시지 처리
void handle_client_message(int fd, int epfd);

#endif // PROTOCOL_H