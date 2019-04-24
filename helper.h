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
void remove_newline_char(char **str);
void replace_other_whitespace(char **str);
bool find_array_b(char **array,char *element,int i,int j);
bool find_array_l(char **array,char *element);
#endif //PROJECT3C_HELPER_H
