#ifndef NETWORK_H
#define NETWORK_H

#include <sys/epoll.h>

#define DEFAULT_PORT 8080
#define MAX_EVENTS   1024
#define BACKLOG      10

// 네트워크 관련 함수들
int  set_nonblocking(int fd);
int  parse_port_from_args(int argc, char *argv[]);
int  create_and_bind_listener(int port);
int  setup_epoll(int listener);
void handle_new_connection(int listener, int epfd);
void handle_client_message(int fd, int epfd);
void event_loop(int listener, int epfd);
void cleanup(int listener, int epfd);

#endif  // NETWORK_H