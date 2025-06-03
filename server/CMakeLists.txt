add_executable(server
    main.c
    network.c
    protocol.c
    handlers/ping_handler.c
    handlers/echo_handler.c
)
target_link_libraries(server PRIVATE common)
target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/common)