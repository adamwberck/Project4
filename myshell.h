//
// Created by mandr on 2019-03-09.
//

#ifndef PROJECT2_MYSHELL_H
#define PROJECT2_MYSHELL_H

#include <stdbool.h>

void my_cd(int count, char** args);

void remove_newline_char(char **str);

void get_input(char* prompt,char **input);

char** parse_input(char* input,int count);

int get_count(char *input);

void execute(char **args,int count,char extra,bool piping_in);

void my_error();

bool my_built_in(int count, char** args);

void put_info_into_env();

int remove_exe_name(char **str);

char *get_prompt() ;

void my_dir(int count, char **args);

int has_write_redirect(char **args, int count);

void replace_other_whitespace(char **str);

int has_read_redirect(char **args, int count);

void perform_write_redirect(char *const *args, int count, int write_redirect);

void process_input(int count, char *const *parsed_input);

#endif //PROJECT2_MYSHELL_H
