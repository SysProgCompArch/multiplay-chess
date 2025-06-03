add_library(common STATIC
    common.c
    common.h
    piece.c
    piece.h
    rule.c
    rule.h
    utils.c
    utils.h
    network.c
    network.h
    logger.c
    logger.h
)

# Proto 파일 경로와 출력 디렉토리 지정
set(PROTO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/proto/message.proto)
# timestamp.proto 파일을 여러 경로에서 찾기
find_path(PROTOBUF_INCLUDE_DIR google/protobuf/timestamp.proto
    PATHS
        /usr/include
        /usr/local/include
        ${PROTOBUF_C_INCLUDE_DIRS}
)
if(PROTOBUF_INCLUDE_DIR)
    set(TIMESTAMP_PROTO_FILE ${PROTOBUF_INCLUDE_DIR}/google/protobuf/timestamp.proto)
else()
    message(FATAL_ERROR "Could not find google/protobuf/timestamp.proto")
endif()
set(GENERATED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/generated)

# 생성될 파일 이름 지정 (message.pb-c.c, message.pb-c.h)
get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
set(GENERATED_SRC ${GENERATED_DIR}/${PROTO_NAME}.pb-c.c)
set(GENERATED_HDR ${GENERATED_DIR}/${PROTO_NAME}.pb-c.h)

# timestamp 관련 생성 파일
set(TIMESTAMP_GENERATED_SRC ${GENERATED_DIR}/google/protobuf/timestamp.pb-c.c)
set(TIMESTAMP_GENERATED_HDR ${GENERATED_DIR}/google/protobuf/timestamp.pb-c.h)

# timestamp proto 파일로부터 C 코드 생성
add_custom_command(
    OUTPUT ${TIMESTAMP_GENERATED_SRC} ${TIMESTAMP_GENERATED_HDR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIR}/google/protobuf
    COMMAND protoc-c --c_out=${GENERATED_DIR} -I${PROTOBUF_INCLUDE_DIR} ${TIMESTAMP_PROTO_FILE}
    DEPENDS ${TIMESTAMP_PROTO_FILE}
    COMMENT "Generating C source from timestamp.proto"
)

# proto 파일로부터 C 코드 생성
add_custom_command(
    OUTPUT ${GENERATED_SRC} ${GENERATED_HDR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIR}
    COMMAND protoc-c --c_out=${GENERATED_DIR} -I${CMAKE_CURRENT_SOURCE_DIR}/proto -I${PROTOBUF_INCLUDE_DIR} ${PROTO_FILE}
    DEPENDS ${PROTO_FILE} ${TIMESTAMP_GENERATED_SRC} ${TIMESTAMP_GENERATED_HDR}
    COMMENT "Generating C source from ${PROTO_FILE}"
)

# 생성된 파일을 common 라이브러리에 추가
target_sources(common PRIVATE ${GENERATED_SRC} ${GENERATED_HDR} ${TIMESTAMP_GENERATED_SRC} ${TIMESTAMP_GENERATED_HDR})

# include 경로에 generated 폴더 추가
target_include_directories(common PUBLIC ${GENERATED_DIR})

# protobuf-c 설정
target_include_directories(common PUBLIC ${PROTOBUF_C_INCLUDE_DIRS})
target_link_libraries(common PUBLIC ${PROTOBUF_C_LIBRARIES})
target_compile_options(common PUBLIC ${PROTOBUF_C_CFLAGS_OTHER})

# ncursesw 설정
target_include_directories(common PUBLIC ${NCURSES_INCLUDE_DIRS})
target_link_libraries(common PUBLIC ${NCURSES_LIBRARIES})
target_compile_options(common PUBLIC ${NCURSES_CFLAGS_OTHER})