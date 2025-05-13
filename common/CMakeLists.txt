add_library(common STATIC
    common.c
    common.h
    piece.c
    piece.h
    rule.c
    rule.h
    utils.c
    utils.h
)
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/external/cJSON) 