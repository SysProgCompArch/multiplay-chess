#ifndef NETWORK_H
#define NETWORK_H

#include <sys/epoll.h>
#include "../../common/generated/message.pb-c.h"  // GameEndType 정의
#include "handlers/match_manager.h"               // ActiveGame 정의

#define DEFAULT_PORT 8080
#define MAX_EVENTS   1024
#define BACKLOG      10
// 게임 종료 브로드캐스트
void broadcast_game_end(ActiveGame *game,
                        GameEndType      type,
                        const char     *message);
// 네트워크 관련 함수들
int  set_nonblocking(int fd);
int  parse_port_from_args(int argc, char *argv[]);
int  create_and_bind_listener(int port);
int  setup_epoll(int listener);
void handle_new_connection(int listener, int epfd);
void handle_client_message(int fd, int epfd);
void event_loop(int listener, int epfd);
void cleanup(int listener, int epfd);

// 에러 응답 헬퍼 함수
int send_error_response(int fd, int error_code, const char *error_message);

#endif  // NETWORK_H