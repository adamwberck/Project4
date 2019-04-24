//
// Created by mandr on 2019-03-09.
//

#ifndef PROJECT4_MYSHELL_H
#define PROJECT4_MYSHELL_H

#include <stdbool.h>

char** parse_file_and_extension(char *input);


void remove_newline_char(char **str);

void get_input(char* prompt,char **input);

char** parse_input(char* input,int count);

int get_count(char *input);

bool my_built_in(char** args);


char *get_prompt() ;


void replace_other_whitespace(char **str);

#endif //PROJECT4_MYSHELL_H
