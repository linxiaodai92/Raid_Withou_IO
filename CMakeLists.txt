cmake_minimum_required(VERSION 3.7)
project(SFVC)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES LFSC.cpp filetable.cpp filetable.h LFSC.h)
add_executable(SFVC ${SOURCE_FILES})
target_link_libraries(SFVC pthread)