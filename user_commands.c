//
// Created by mandr on 2019-04-21.
//

#include "user_commands.h"
#include "disk.h"

MY_FILE *move_directory(MY_FILE *new_folder, MY_FILE *parent,MY_FILE *file,char name[NAME_LENGTH]){
    return move_file(new_folder,parent,file,name,"\\\\\\");
}

MY_FILE *user_move_file(MY_FILE *new_folder, MY_FILE *parent,MY_FILE *file,char name[NAME_LENGTH],char ext[EXT_LENGTH]){
    if(strstr(name,"\\")==NULL && strstr(ext,"\\")==NULL) {
        MY_FILE *new_file = copy_file(new_folder,file,name,ext);
        delete_file(parent,name,ext);
        return new_file;
    }
    return NULL;
}

MY_FILE *copy_dir(MY_FILE *new_folder,MY_FILE *duplicating_file,char name[NAME_LENGTH]){
    return copy_file( new_folder, duplicating_file, name, "\\\\\\");
}

//copy file that doesn't allow real files to be copied as folders
MY_FILE *user_copy_file(MY_FILE *new_folder,MY_FILE *file,char name[NAME_LENGTH],char ext[EXT_LENGTH]){
    if(strstr(name,"\\")==NULL && strstr(ext,"\\")==NULL) {
        return copy_file( new_folder, file, name, ext);
    }
    return NULL;
}


MY_FILE *make_dir(MY_FILE *parent,char *name){
    char *data={NULL};
    return create_file( parent, name, "\\\\\\", data, 0);
}


MY_FILE *user_create_file(MY_FILE *parent,char *name,char *ext,char *data,uint16_t size){
    if(strstr(name,"\\")==NULL && strstr(ext,"\\")==NULL) {
        return create_file(parent, name, ext, data, size);
    }
    return NULL;
}