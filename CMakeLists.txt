cmake_minimum_required(VERSION 3.15)
project(multiplay-chess C)

# 필요한 라이브러리들을 전역적으로 찾기
find_package(PkgConfig REQUIRED)
pkg_check_modules(NCURSES REQUIRED ncursesw)
pkg_check_modules(PROTOBUF_C REQUIRED libprotobuf-c)

add_subdirectory(common)
add_subdirectory(client)
add_subdirectory(server)