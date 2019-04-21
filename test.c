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

    //srandom((unsigned int) time(NULL));
    MY_FILE *file[35];
    for(int i=0;i<35;i++) {
        uint16_t start = (uint16_t) (random() % (strlen(my_test_file_data)-100));
        uint16_t bytes = (uint16_t) (random() % (strlen(my_test_file_data)-start));
        char name[9]="\0\0\0\0\0\0\0\0\0";
        memcpy(name,"test",4);
        char str[3];
        sprintf(str,"%d",i);
        strcat(name,str);
        printf("%s %d \n",name,bytes);
        file[i] = create_file(&root_folder, name, "txt", my_test_file_data+start, bytes);
    }


    //delete_file(&root_folder,"test30","txt");
    MY_FILE *folder1 = make_dir(&root_folder,"folder1\0\0");
    MY_FILE *ftest1
        = user_create_file(folder1,"ftest1\0\0\0","txt","Hello World! this is a test string in an interior folder",56);
    //delete_file(disk,&root_file,"folder1","\\\\\\");
    //create_file(disk, &root_folder, "new file", "txt", my_test_file_data, 1050);
    //move_file(disk,folder1,&root_folder,file[10],"test9","txt");
    MY_FILE *folder1_2 = make_dir(folder1,"fol1_2");
    user_create_file(folder1_2,"ftest3","txt","file in folder in a folder\n",27);
    copy_file(folder1_2,file[30],"test29","txt");
    open_file(&root_folder,"test0","txt");

    //read file
    uint16_t read_amount = 550;
    char *test_data = malloc((read_amount+1)*sizeof(char));
    MY_FILE *o_file = open_file(folder1_2,"ftest3","txt");
    while(!o_file->isEOF) {
        int bytes_written = read_data( o_file, test_data, read_amount);
        test_data[bytes_written] = '\0';
        printf("%s", test_data);
    }
    free(test_data);
    close_file(o_file);
    printf("folder1's fat %d\n",folder1->FAT_LOC);
    printf("ftest1's fat %d\n",ftest1->FAT_LOC);
    printf("fol1_2's fat %d\n",folder1_2->FAT_LOC);
    //delete_file(&root_folder,"folder1","\\\\\\");
    MY_FILE *folder2 = make_dir(&root_folder,"folder2");
    user_create_file(folder2,"new_file","txt","Once upon a time there was a test file\n",39);
    copy_file(folder2,folder1,"folder1c","\\\\\\");
}

void second_test(){

}