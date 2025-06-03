#include <stdio.h>
#include <stdlib.h>
#include "network.h"

int main(int argc, char *argv[])
{
    int port = parse_port_from_args(argc, argv);
    int listener = create_and_bind_listener(port);
    int epfd = setup_epoll(listener);

    printf("epoll server started (port: %d)\n", port);

    event_loop(listener, epfd);
    cleanup(listener, epfd);

    return 0;
}
