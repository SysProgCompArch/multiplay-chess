include_directories(${CMAKE_SOURCE_DIR}/common/generated)
add_executable(server
    server.c
    $<TARGET_OBJECTS:common_objs>
)
target_link_libraries(server PRIVATE common cjson protobuf-c)
target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/common ${CMAKE_SOURCE_DIR}/external/cJSON)