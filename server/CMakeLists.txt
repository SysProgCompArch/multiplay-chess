add_executable(server
    main.c
    server_network.c
    match_manager.c
    handlers/dispatcher.c
    handlers/ping.c
    handlers/echo.c
    handlers/match.c
    handlers/chat.c
    handlers/move.c
)
target_link_libraries(server PRIVATE common pthread)
target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/common)