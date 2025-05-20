add_executable(server
    server.c
)
target_link_libraries(server PRIVATE common)
target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/common)