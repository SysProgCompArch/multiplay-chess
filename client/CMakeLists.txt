add_executable(client
    client.c
)

target_link_libraries(client PRIVATE common)
target_include_directories(client PRIVATE ${CMAKE_SOURCE_DIR}/common)