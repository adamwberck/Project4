//
// Created by Adam Berck on 2019-04-21.
//

#include "user_commands.h"
#include "disk.h"

MY_FILE *move_directory(MY_FILE *new_folder, MY_FILE *parent,MY_FILE *file,char name[NAME_LENGTH]){
    name = uniform_the_name(name);
    MY_FILE *m_file = move_file(new_folder,parent,file,name,"\\\\\\");
    free(name);
    return m_file;
}

MY_FILE *user_move_file(MY_FILE *new_folder, MY_FILE *parent,MY_FILE *file,char name[NAME_LENGTH],char ext[EXT_LENGTH]){
    name = uniform_the_name(name);
    ext = uniform_the_ext(ext);
    if(strstr(name,"\\")==NULL && strstr(ext,"\\")==NULL) {
        MY_FILE *new_file = copy_file(new_folder,file,name,ext);
        delete_file(parent,name,ext);
        free(name);free(ext);
        return new_file;
    }
    return NULL;
}

MY_FILE *copy_dir(MY_FILE *new_folder,MY_FILE *duplicating_file,char name[NAME_LENGTH]){
    name = uniform_the_name(name);
    MY_FILE *c_file = copy_file( new_folder, duplicating_file, name, "\\\\\\");
    free(name);
    return c_file;
}

char *uniform_the_name(char name[NAME_LENGTH]){
    char *u_name =malloc(NAME_LENGTH* sizeof(char));
    strcpy(u_name,name);
    int len = (int) strlen(name);
    while(len<NAME_LENGTH){
        u_name[len++]=0;
    }
    return u_name;
}

char *uniform_the_ext(char ext[EXT_LENGTH]){
    char *u_ext =malloc(EXT_LENGTH* sizeof(char));
    strcpy(u_ext,ext);
    int len = (int) strlen(ext);
    while(len<EXT_LENGTH){
        u_ext[len++]=0;
    }
    return u_ext;
}

//copy file that doesn't allow real files to be copied as folders
MY_FILE *user_copy_file(MY_FILE *new_folder,MY_FILE *file,char name[NAME_LENGTH],char ext[EXT_LENGTH]){
    name = uniform_the_name(name);
    ext = uniform_the_ext(ext);
    if(strstr(name,"\\")==NULL && strstr(ext,"\\")==NULL) {
        MY_FILE *c_file = copy_file( new_folder, file, name, ext);
        free(name);free(ext);
        return c_file;
    }
    free(name);free(ext);
    return NULL;
}



MY_FILE *make_dir(MY_FILE *parent, char *name){
    name = uniform_the_name(name);
    char *data={NULL};
    MY_FILE *new_dir = create_file( parent, name, "\\\\\\", data, 0);
    free(name);
    return new_dir;
}


MY_FILE *user_create_file(MY_FILE *parent,char *name,char *ext,char *data,uint16_t size){
    name = uniform_the_name(name);
    ext  = uniform_the_ext(ext);
    if(strstr(name,"\\")==NULL && strstr(ext,"\\")==NULL) {
        MY_FILE *new_file = create_file(parent, name, ext, data, size);
        free(name);free(ext);
        return new_file;
    }
    free(name);free(ext);
    return NULL;
}

void display_file_data(MY_FILE *file){
    file->data_loc=0;
    file->isEOF=false;
    while(!file->isEOF){
        char data[file->DATA_SIZE];
        read_data(file,data,file->DATA_SIZE);
        printf("%s",data);
    }
}