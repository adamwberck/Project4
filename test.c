//
// Created by mandr on 2019-04-20.
//

#include <stdio.h>
#include <malloc.h>
#include "test.h"

void first_test(struct boot my_boot){
    //Load Test File.txt to write to disk
    FILE *test_file = fopen("Test File2.txt","r");
    int test_file_size = (int) fsize("Test File2.txt");
    printf("size %d\n",  test_file_size);
    char *my_test_file_data = malloc(sizeof(char)*test_file_size);
    fprintf(test_file,"%s",my_test_file_data);
    fread(my_test_file_data, sizeof(char), (size_t) test_file_size, test_file);
    fclose(test_file);

    //create test file write to logic dir, write to fat, and write to disk
    MY_FILE root_folder;
    root_folder.FAT_LOC=my_boot.root.FAT_location;
    root_folder.isEOF=false;
    root_folder.data_loc=0;
    root_folder.DATA_SIZE=my_boot.root.size;

    srandom((unsigned int) time(NULL));
    MY_FILE *folders[6];
    folders[0] = make_dir(&root_folder, "folder0\0\0");
    folders[1] = make_dir(&root_folder, "folder1\0\0");
    folders[2] = make_dir(&root_folder, "folder2\0\0");
    folders[3] = make_dir(&root_folder, "folder3\0\0");
    folders[4] = make_dir(&root_folder, "folder4\0\0");
    folders[5] = make_dir(&root_folder, "folder5\0\0");
    MY_FILE *file[35];

    for(int i=0;i<30;i++) {
        uint16_t start = (uint16_t) (random() % (strlen(my_test_file_data)-100));
        uint16_t bytes = (uint16_t) (random() % (strlen(my_test_file_data)-start));
        uint16_t folder = (uint16_t) (random() % 7);
        char name[9]="\0\0\0\0\0\0\0\0\0";
        memcpy(name,"test",4);
        char str[3];
        sprintf(str,"%.2d",i);
        strcat(name,str);
        //printf("%s %d \n",name,bytes);
        if(folder!=6) {
            file[i] = user_create_file(folders[folder], name, "txt", my_test_file_data + start, bytes);
        }
        else {
            file[i] = user_create_file(&root_folder, name, "c", my_test_file_data + start, bytes);
        }
    }
}

void second_test(){
    make_dir(current_dir,"folder1");
    user_create_file(current_dir,"testA","txt","This is a test of file creation",31);
    change_dir("folder1");
    user_create_file(current_dir,"test0","txt","This is 0 test of file creation",31);
    user_create_file(current_dir,"test1","txt","This is 1 test of file creation",31);
    make_dir(current_dir,"sub1");
    change_dir("sub1");
    user_create_file(current_dir,"test2","txt","This is 2 test of file creation",31);
    MY_FILE *test3 = user_create_file(current_dir,"test3","txt","This is 3 test of file creation",31);
    display_everything();
    user_delete_file(current_dir,"test2","txt");
    change_dir("..");
    change_dir("..");
    user_delete_file(current_dir,"testA","txt");
    display_everything();
}