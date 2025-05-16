cmake_minimum_required(VERSION 3.15) # FetchContent를 위한 최소 버전

include(FetchContent)

# protobuf-c 라이브러리를 외부 의존성으로 추가
FetchContent_Declare(
    protobuf_c
    GIT_REPOSITORY https://github.com/protobuf-c/protobuf-c.git
    GIT_TAG master # 또는 특정 버전 태그(예: v1.5.0)로 변경 가능
)

# 의존성 사용 가능하게 만들기
FetchContent_MakeAvailable(protobuf_c)

add_library(common STATIC
    common.c
    common.h
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/proto)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/generated)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/generated)
# protobuf-c 헤더 포함 경로 추가
include_directories(${protobuf_c_SOURCE_DIR})

# 시스템에 설치된 protoc-c 실행 파일 찾기
find_program(PROTOC_C protoc-c)
if(NOT PROTOC_C)
    message(FATAL_ERROR "protoc-c not found. Please install protobuf-c-compiler package")
endif()

# 1. proto 파일 목록 가져오기
file(GLOB PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto")

set(GENERATED_SRCS)
set(GENERATED_HDRS)

foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    set(GEN_SRC "${CMAKE_CURRENT_SOURCE_DIR}/generated/${PROTO_NAME}.pb-c.c")
    set(GEN_HDR "${CMAKE_CURRENT_SOURCE_DIR}/generated/${PROTO_NAME}.pb-c.h")

    # 2. proto -> c/h 변환 명령 추가 (시스템 protoc-c 사용)
    add_custom_command(
        OUTPUT ${GEN_SRC} ${GEN_HDR}
        COMMAND ${PROTOC_C} --c_out=${CMAKE_CURRENT_SOURCE_DIR}/generated -I${CMAKE_CURRENT_SOURCE_DIR}/proto ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "Generating ${GEN_SRC} and ${GEN_HDR} from ${PROTO_FILE}"
        VERBATIM
    )

    list(APPEND GENERATED_SRCS ${GEN_SRC})
    list(APPEND GENERATED_HDRS ${GEN_HDR})
endforeach()

# 3. 전체 proto 빌드 타겟
add_custom_target(generate_proto ALL DEPENDS ${GENERATED_SRCS} ${GENERATED_HDRS})

# 4. common_objs에 생성된 파일 포함
add_library(common_objs OBJECT common.c ${GENERATED_SRCS})

add_dependencies(common_objs generate_proto)

# protobuf-c 라이브러리에 대한 의존성 추가
target_link_libraries(common PRIVATE protobuf-c)