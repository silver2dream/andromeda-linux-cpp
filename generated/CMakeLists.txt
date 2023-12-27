include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB SOURCES "*.cc")
add_library(generated ${SOURCES})