include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB SOURCES "*.cxx")
add_library(misc ${SOURCES})