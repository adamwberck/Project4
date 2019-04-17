cmake_minimum_required(VERSION 3.12)
project(Project4 C)

set(CMAKE_C_STANDARD 99)

add_executable(Project4 myshell.c myshell.h helper.c helper.h main.c main.h my_stack.c my_stack.h)