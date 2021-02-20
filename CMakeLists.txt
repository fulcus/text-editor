cmake_minimum_required(VERSION 3.16)
project(text_editor C)

set(CMAKE_C_STANDARD 11)

#added compiler flags
set(GCC_COVERAGE_COMPILE_FLAGS "-DEVAL -std=gnu11 -O0 -pipe -lm -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
#end compiler flags

add_executable(text_editor main.c)