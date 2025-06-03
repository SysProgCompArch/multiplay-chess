add_executable(client
    main.c
    game_state.c
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