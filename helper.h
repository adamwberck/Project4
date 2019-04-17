//
// Created by Adam Berck on 2019-03-28.
//

#ifndef PROJECT3C_HELPER_H
#define PROJECT3C_HELPER_H
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

void print_array(char **array);
void remove_newline_char(char **str);
void read_file_as_array(char ***array, FILE *file);
void replace_other_whitespace(char **str);
int count_file_lines(FILE *file);
int open_listen_fd(int port);
int remove_exe_name(char **str);
bool find_in_array(char **array,char *element,bool in_order,int size);
bool find_array_b(char **array,char *element,int i,int j);
bool find_array_l(char **array,char *element);
bool array_in_order(char** array);
#endif //PROJECT3C_HELPER_H
