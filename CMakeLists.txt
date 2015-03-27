cmake_minimum_required(VERSION 2.6)

project(uncbv)

set(CMAKE_CXX_FLAGS "-std=c++14")
set(CMAKE_CXX_FLAGS_DEBUG "-pedantic -Wall -Wextra -Wold-style-cast -Woverloaded-virtual -Wfloat-equal -Wwrite-strings -Wpointer-arith -Wcast-qual -Wcast-align -Wconversion -Wshadow -Weffc++ -Wredundant-decls -Wdouble-promotion -Winit-self -Wswitch-default -Wswitch-enum -Wundef -Wlogical-op -Winline -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -Os -s")

file(
    GLOB_RECURSE
    source_files
    src/*
)

add_executable(
    ${PROJECT_NAME}
    ${source_files}
)
