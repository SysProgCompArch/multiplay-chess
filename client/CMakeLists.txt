include_directories(${CMAKE_SOURCE_DIR}/common/generated)
add_executable(client client.c $<TARGET_OBJECTS:common_objs>)
target_link_libraries(client PRIVATE common cjson protobuf-c)
target_include_directories(client PRIVATE ${CMAKE_SOURCE_DIR}/common ${CMAKE_SOURCE_DIR}/external/cJSON) 