cmake_minimum_required(VERSION 3.12)
project(Project4 C)

set(CMAKE_C_STANDARD 99)

add_executable(Project4 myshell.c myshell.h disk.c disk.h my_stack.c my_stack.h
        test.c test.h my_dir_stack.c my_dir_stack.h user_commands.c user_commands.h helper.c helper.h)