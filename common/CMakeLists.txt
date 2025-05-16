add_library(common STATIC
    common.c
    common.h
)
target_include_directories(common PUBLIC)

target_include_directories(common PUBLIC /usr/include/protobuf-c)
target_link_libraries(common PUBLIC protobuf-c)