cmake_minimum_required(VERSION 3.12)
project(andromeda-linux-cpp)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(BUILD_ROOT ${CMAKE_SOURCE_DIR})

include_directories(${BUILD_ROOT}/_include)
include_directories(${BUILD_ROOT}/generated)

add_subdirectory(${BUILD_ROOT}/signal)
add_subdirectory(${BUILD_ROOT}/proc)
add_subdirectory(${BUILD_ROOT}/net)
add_subdirectory(${BUILD_ROOT}/misc)
add_subdirectory(${BUILD_ROOT}/generated)
add_subdirectory(${BUILD_ROOT}/business)
add_subdirectory(${BUILD_ROOT}/app)

add_executable(andromeda app/andromeda.cxx)

target_link_libraries(andromeda app signal business proc net misc generated protobuf)

configure_file(${CMAKE_SOURCE_DIR}/andromeda.conf ${CMAKE_BINARY_DIR}/andromeda.conf COPYONLY)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/logs)

add_custom_target(clear
        COMMAND ${CMAKE_COMMAND} -E remove "andromeda"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${BUILD_ROOT}/log.txt"
)

add_custom_target(memcheck
        COMMAND valgrind --tool=memcheck --leak-check=full --show-reachable=yes --trace-children=yes --log-file=${BUILD_ROOT}/log.txt ./andromeda
)

add_custom_target(protoc
        COMMAND protoc -I=/projects/andromeda-linux-cpp/proto --cpp_out=./generated /projects/andromeda-linux-cpp/proto/*.proto
)

add_custom_target(kill
        COMMAND ${CMAKE_COMMAND} -E remove "logs/error.log"
        COMMAND pgrep andromeda | xargs kill -9
)