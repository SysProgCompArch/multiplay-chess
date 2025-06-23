add_executable(server
    main.c
    server_network.c
    handlers/dispatcher.c
    handlers/ping.c
    handlers/echo.c
    handlers/match.c
    handlers/match_manager.c
    handlers/chat.c
    handlers/move.c
    handlers/timeout.c 

)
target_link_libraries(server PRIVATE common pthread)
target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/common)