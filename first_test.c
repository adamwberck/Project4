//
// Created by mandr on 2019-04-20.
//

#include <stdio.h>
#include <malloc.h>
#include "first_test.h"

void first_test(void * disk,struct boot my_boot){
    //Load Test File.txt to write to disk
    FILE *test_file = fopen("Test File2.txt","r");
    int test_file_size = (int) fsize("Test File2.txt");
    printf("size %d\n",  test_file_size);
    char *my_test_file_data = malloc(sizeof(char)*test_file_size);
    fprintf(test_file,"%s",my_test_file_data);
    fread(my_test_file_data, sizeof(char), (size_t) test_file_size, test_file);
    fclose(test_file);

    //create test file write to logic dir, write to fat, and write to disk
    MY_FILE root_pointer;
    root_pointer.FAT_LOC=my_boot.root.FAT_location;
    root_pointer.isEOF=false;
    root_pointer.data_loc=0;
    root_pointer.DATA_SIZE=my_boot.root.size;

    //srandom((unsigned int) time(NULL));
    MY_FILE *file[35];
    for(int i=0;i<35;i++) {
        uint16_t start = (uint16_t) (random() % (strlen(my_test_file_data)-100));
        uint16_t bytes = (uint16_t) (random() % (strlen(my_test_file_data)-start));
        char name[7];
        strcpy(name,"test");
        char str[3];
        sprintf(str,"%d",i);
        strcat(name,str);
        strcat(name,"\0");
        printf("%s %d \n",name,bytes);
        file[i] = create_file(disk, &root_pointer, name, "txt", my_test_file_data+start, bytes);
    }


    //delete_file(disk,&root_file,"test30","txt");
    MY_FILE *folder1 = make_dir(disk,&root_pointer,"folder1");
    user_create_file(disk,folder1,"ftest1","txt","Hello World! this is a test string in an interior folder",56);
    //delete_file(disk,&root_file,"folder1","\\\\\\");
    //create_file(disk, &root_pointer, "new file", "txt", my_test_file_data, 1050);
    move_file(disk,folder1,&root_pointer,file[10],"test9","txt");
    MY_FILE *folder1_2 = make_dir(disk,folder1,"fol1_2");
    user_create_file(disk,folder1_2,"ftest3","txt","file in folder in a folder",26);
    copy_file(disk,folder1_2,file[30],"test29","txt");
    open_file(disk,&root_pointer,"test0","txt");

    //read file
    uint16_t read_amount = 550;
    char *test_data = malloc((read_amount+1)*sizeof(char));
    MY_FILE *o_file = open_file(disk,folder1_2,"ftest3","txt");
    while(!o_file->isEOF) {
        int bytes_written = read_data(disk, o_file, test_data, read_amount);
        test_data[bytes_written] = '\0';
        printf("%s", test_data);
    }
    free(test_data);
}