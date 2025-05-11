add_executable(server
    server.c
)
target_link_libraries(server PRIVATE common cjson)
target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/common ${CMAKE_SOURCE_DIR}/external/cJSON)