cmake_minimum_required(VERSION 3.7)
project(SFVC)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        file_table.cpp
        file_table.h
         main.cpp)

add_executable(SFVC ${SOURCE_FILES})