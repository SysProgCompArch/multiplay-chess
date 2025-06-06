add_executable(client
    main.c
    game_state.c
    client_state.c
    client_network.c
    ui/core.c
    ui/screens.c
    ui/input.c
    ui/chat.c
    ui/colors.c
    handlers/ping.c
    handlers/echo.c
    handlers/match.c
    handlers/chat.c
    handlers/error.c
    handlers/disconnect.c
    handlers/dispatcher.c
)

# ncurses와 pthread 라이브러리 찾기
find_package(PkgConfig REQUIRED)
pkg_check_modules(NCURSES REQUIRED ncursesw)
find_package(Threads REQUIRED)

target_link_libraries(client PRIVATE 
    common
    ${NCURSES_LIBRARIES}
    Threads::Threads
)

target_include_directories(client PRIVATE 
    ${CMAKE_SOURCE_DIR}/common
    ${NCURSES_INCLUDE_DIRS}
)

target_compile_options(client PRIVATE ${NCURSES_CFLAGS_OTHER})

# test-client 실행파일 추가
add_executable(test-client
    test-client.c
)

target_link_libraries(test-client PRIVATE 
    common
)

target_include_directories(test-client PRIVATE 
    ${CMAKE_SOURCE_DIR}/common
)