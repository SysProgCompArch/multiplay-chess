add_library(common STATIC
    common.c
    common.h
)
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/external/cJSON)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/proto)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/generated)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/generated)

file(GLOB GENERATED_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/generated/*.c")
add_library(common_objs OBJECT common.c ${GENERATED_SRCS})

# proto 파일 자동 컴파일 (protoc-c)
find_program(PROTOC_C protoc-c)
file(GLOB PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto")
set(GENERATED_FILES)
foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    set(GENERATED_C   "${CMAKE_CURRENT_SOURCE_DIR}/generated/${PROTO_NAME}.pb-c.c")
    set(GENERATED_H   "${CMAKE_CURRENT_SOURCE_DIR}/generated/${PROTO_NAME}.pb-c.h")
    add_custom_command(
        OUTPUT ${GENERATED_C} ${GENERATED_H}
        COMMAND ${PROTOC_C} -I=${CMAKE_CURRENT_SOURCE_DIR}/proto --c_out=${CMAKE_CURRENT_SOURCE_DIR}/generated ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "Running protoc-c on ${PROTO_FILE}"
        VERBATIM
    )
    list(APPEND GENERATED_FILES ${GENERATED_C} ${GENERATED_H})
endforeach()

add_custom_target(proto_gen ALL DEPENDS ${GENERATED_FILES})
add_dependencies(common_objs proto_gen) 