add_executable(server
    main.c
    server_network.c
    handlers/dispatcher.c
    handlers/ping_handler.c
    handlers/echo_handler.c
    handlers/match_handler.c
    handlers/match_manager.c
)
target_link_libraries(server PRIVATE common pthread)
target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/common)