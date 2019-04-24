//
// Created by mandr on 2019-04-21.
//

#ifndef PROJECT4_USER_COMMANDS_H
#define PROJECT4_USER_COMMANDS_H
#include "disk.h"
MY_FILE *move_directory(MY_FILE *new_folder, MY_FILE *parent,MY_FILE *file,char name[NAME_LENGTH]);
MY_FILE *user_move_file(MY_FILE *new_folder, MY_FILE *parent,MY_FILE *file,char name[NAME_LENGTH],char ext[EXT_LENGTH]);
MY_FILE *copy_dir(MY_FILE *new_folder,MY_FILE *duplicating_file,char name[NAME_LENGTH]);
MY_FILE *user_copy_file(MY_FILE *new_folder,MY_FILE *file,char name[NAME_LENGTH],char ext[EXT_LENGTH]);
MY_FILE *make_dir(MY_FILE *parent, char *name);
MY_FILE *user_create_file(MY_FILE *parent,char *name,char *ext,char *data,uint16_t size);
void display_file_data(MY_FILE *file);
char *uniform_the_ext(char *ext);
char *uniform_the_name(char *name);
void user_delete_file(MY_FILE *parent, char *name, char *ext );
void delete_dir(MY_FILE *parent, char *name );
MY_FILE *user_open_file(MY_FILE *parent,char name[NAME_LENGTH],char ext[EXT_LENGTH]);
#endif //PROJECT4_USER_COMMANDS_H
