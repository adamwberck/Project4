//
// Created by Adam Berck on 2019-04-17.
//

#include <stdio.h>
#include <memory.h>
#include <time.h>
#include "main.h"
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "my_stack.h"

#define FREE_BLOCK   0x0000
#define NO_LINK 0xFFFF

#define BOOT_SECTOR_SIZE 512
#define TOTAL_SIZE 2113016
#define BLOCK_SIZE 512
#define TOTAL_BLOCKS 4094
#define FAT_SIZE 8188
#define MAX_SIZE 65535

//entry is 32 bytes
#define ENTRY_SIZE 32

//root location is 0x26
#define ROOT_LOCATION 0x26
#define FAT1_LOCATION 0x200
#define FAT2_LOCATION 0x21FC
#define USER_SPACE_LOCATION 0x41F8

#define NAME_LENGTH 9
#define EXT_LENGTH 3

struct MY_FILE {
    uint16_t data_loc;
    uint16_t DATA_SIZE;
    uint16_t FAT_LOC;
    uint16_t pFAT_LOC;
    bool isEOF;
}typedef MY_FILE;


struct dir_entry{
    char name[NAME_LENGTH];
    char extension[EXT_LENGTH];
    uint16_t size;
    time_t create_time;
    time_t mod_time;
    uint16_t FAT_location;
};

struct boot{
    char valid_check[24];
    uint16_t boot_sector_size; //512
    uint32_t total_size; //2096128
    uint16_t block_size; //512
    uint16_t total_blocks; //4094
    uint16_t FAT_size; //8188
    uint16_t max_size; //65535
    struct dir_entry root;
};


void entry_to_data(struct dir_entry entry,char array[ENTRY_SIZE]);
void delete_file(void *disk,MY_FILE *parent, char filename[NAME_LENGTH],char ext[EXT_LENGTH] );
uint16_t write_data(void *disk, struct MY_FILE *p_file, void *data, uint16_t bytes);
uint16_t block_pos(void *disk,uint16_t fat_loc,uint16_t data_loc);
uint32_t get_disk_pos(void *disk, uint16_t fat_loc, uint16_t data_loc);
MY_FILE *create_file(void *disk,MY_FILE *parent, char name[NAME_LENGTH],char ext[EXT_LENGTH],
                     char *data,uint16_t size);
void write_file_to_fat(struct dir_entry entry,void *disk);
void write_dir_entry(struct dir_entry entry,void *disk,uint32_t location);
uint16_t fat_value(void* disk, uint16_t block);
uint32_t fat_location(bool isFAT1, uint16_t block);
struct dir_entry create_root();
struct boot create_boot();
struct dir_entry create_entry(char name[9],char extension[3],uint16_t size, time_t create_time,
        time_t mod_time,uint16_t FAT_location);


uint16_t get_free_block(void *disk,uint16_t start);

uint16_t read_data(void *disk,struct MY_FILE *p_file,void *data, uint16_t bytes);
off_t fsize(const char *filename);

int main(){
    FILE *new_disk = fopen("my_disk","w+");
    uint16_t empty = FREE_BLOCK;
    for(int i=0;i<TOTAL_SIZE;i++) {
        fwrite(&empty, sizeof(uint16_t), 1, new_disk);
    }
    fseek(new_disk,0,SEEK_SET);
    struct boot my_boot = create_boot();
    fwrite(my_boot.valid_check,1, sizeof(my_boot.valid_check),new_disk);
    fwrite(&my_boot.boot_sector_size,2, 1,new_disk);
    fwrite(&my_boot.total_size,4, 1,new_disk);
    fwrite(&my_boot.block_size,2, 1,new_disk);
    fwrite(&my_boot.total_blocks,2, 1,new_disk);
    fwrite(&my_boot.FAT_size,2, 1,new_disk);
    fwrite(&my_boot.max_size,2, 1,new_disk);
    //write_dir_entry(my_boot.root,new_disk);
    fclose(new_disk);

    //COPY my_test_file_data using mmap();
    int i_disk = open("my_disk",O_RDWR,0);
    void *disk = mmap(NULL,TOTAL_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,i_disk,0);
    write_dir_entry(my_boot.root,disk,ROOT_LOCATION);
    write_file_to_fat(my_boot.root,disk);

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
    for(int i=0;i<100;i++) {
        uint16_t start = (uint16_t) (random() % (strlen(my_test_file_data)-100));
        uint16_t bytes = (uint16_t) (random() % (strlen(my_test_file_data)-start));
        char name[7];
        strcpy(name,"test");
        char str[3];
        sprintf(str,"%d",i);
        strcat(name,str);
        strcat(name,"\0");
        printf("%s %d \n",name,bytes);
        MY_FILE *file = create_file(disk, &root_pointer, name, "txt", my_test_file_data+start, bytes);
    }
    uint16_t read_amount = 550;
    char *test_data = malloc((read_amount+1)*sizeof(char));

    //read file
    /*
    while(!file->isEOF) {
        int bytes_written = read_data(disk, file, test_data, read_amount);
        test_data[bytes_written] = '\0';
        printf("%s", test_data);
    }
    */

    MY_FILE root_file; root_file.isEOF=false; root_file.DATA_SIZE=my_boot.root.size;
    root_file.FAT_LOC=my_boot.root.FAT_location; root_file.data_loc=0;

    //delete_file(disk,&root_file,"test","txt");
    uint16_t test_fat_scan = root_file.FAT_LOC;
    while(test_fat_scan!=NO_LINK){
        printf("%x\n",test_fat_scan);
        test_fat_scan = fat_value(disk,test_fat_scan);
    }
    delete_file(disk,&root_file,"test30","txt");
}

void delete_file(void *disk,MY_FILE *parent, char *filename,char *ext ){

    //edit size in parent
    //special case if file is in root then the dir information is in the BOOT SECTOR
    uint32_t dir_disk_loc=ROOT_LOCATION;
    if(parent->FAT_LOC!=0) {
        uint16_t check_fat=0;
        uint16_t d_search =0;
        while(d_search<MAX_SIZE-ENTRY_SIZE && check_fat != parent->FAT_LOC) {
            uint32_t d_pos = get_disk_pos(disk, parent->pFAT_LOC, d_search);
            memcpy(&check_fat, disk + d_pos + ENTRY_SIZE - sizeof(uint16_t), sizeof(uint16_t));
            d_search+=ENTRY_SIZE;
        }
        printf("check fat %d real fat %d\n",check_fat,parent->FAT_LOC);
    }

    //BAD if goes across a block
    uint16_t d_size;
    memcpy(&d_size,disk+dir_disk_loc+NAME_LENGTH+EXT_LENGTH, sizeof(uint16_t));
    parent->DATA_SIZE=d_size;
    d_size-=ENTRY_SIZE;
    memcpy(disk+dir_disk_loc+NAME_LENGTH+EXT_LENGTH,&d_size,sizeof(uint16_t));

    //add file info to the directory
    parent->data_loc=0;
    char disk_name[NAME_LENGTH+1]="\0\0\0";
    char disk_ext[EXT_LENGTH+1]="\0\0\0";
    bool n = false;
    bool e = false;
    while(!parent->isEOF && (!n || !e)) {
        read_data(disk, parent, disk_name, NAME_LENGTH);
        read_data(disk, parent, disk_ext,EXT_LENGTH);
        disk_name[NAME_LENGTH]='\0';
        disk_ext[EXT_LENGTH]='\0';
        printf("r %s%s  d %s%s\n",filename,ext,disk_name,disk_ext);
        parent->data_loc+=ENTRY_SIZE-NAME_LENGTH-EXT_LENGTH;
        n = strcmp(filename,disk_name)==0;
        e = strcmp(ext,disk_ext)==0;
    }

    //erase fat
    uint16_t fat_loc;
    parent->data_loc -= sizeof(uint16_t);
    read_data(disk,parent,&fat_loc, sizeof(uint16_t));
    uint16_t first_fat_loc = fat_loc;
    printf("fat %x\n",first_fat_loc);
    struct my_stack stack = create_my_stack();
    put_my_stack(&stack,fat_loc);
    while(fat_loc!=NO_LINK){
        fat_loc = fat_value(disk,fat_loc);
        put_my_stack(&stack,fat_loc);
    }
    //erase dir info
    uint16_t free_block = FREE_BLOCK;
    while(stack.size>0){
        //erase the first fat
        uint32_t f_pos = fat_location(true,pop_my_stack(&stack));
        memcpy(disk+f_pos,&free_block, sizeof(uint16_t));
        //erase the second fat
        f_pos = fat_location(false,pop_my_stack(&stack));
        memcpy(disk+f_pos,&free_block, sizeof(uint16_t));
    }
    printf("fat %x\n",fat_value(disk,first_fat_loc));
    uint16_t old_data_loc = parent->data_loc;
    uint16_t amount_of_data = parent->DATA_SIZE-old_data_loc;
    char rest_of_data[amount_of_data];
    read_data(disk,parent,rest_of_data,amount_of_data);
    parent->data_loc = (uint16_t) (old_data_loc - ENTRY_SIZE);
    write_data(disk,parent,rest_of_data,amount_of_data);

}



off_t fsize(const char *filename) {
    struct stat st;

    if (stat(filename, &st) == 0)
        return st.st_size;

    return -1;
}

//TODO check duplicate file name
uint32_t get_dir_location(void *disk,struct dir_entry parent){
    int temp_size = parent.size;
    uint16_t fat = parent.FAT_location;
    while(temp_size>BLOCK_SIZE){
        fat = fat_value(disk,fat);
        temp_size-=BLOCK_SIZE;
    }
    return (uint32_t) (USER_SPACE_LOCATION + fat * BLOCK_SIZE + temp_size);
}

uint16_t seek_data(void *disk,struct MY_FILE *p_file, uint16_t offset, int whence){
    /*
    uint32_t user_loc = p_file->disk_pos-USER_SPACE_LOCATION; //location in the user space
    uint16_t block_loc = (uint16_t) (user_loc % BLOCK_SIZE); //location in specific block
    uint16_t fat_loc  = (uint16_t) (user_loc / BLOCK_SIZE);*/

}

uint16_t read_data(void *disk,struct MY_FILE *p_file,void *data, uint16_t bytes){
    uint16_t bytes_read = 0;

    uint16_t remaining_bytes = p_file->DATA_SIZE-p_file->data_loc; //amount of bytes left in file

    p_file->isEOF = bytes>=remaining_bytes; //if remaining bytes is less than bytes available set EOF true
    uint16_t  bytes_to_read = bytes<=remaining_bytes ? bytes : remaining_bytes; //only get bytes <= to bytes available

    while(bytes_read<bytes_to_read) {
        //get location of data in block
        uint16_t block_loc = (uint16_t) (p_file->data_loc % BLOCK_SIZE);//block_pos(disk,p_file->FAT_LOC,p_file->data_loc);
        //set bytes to read to rest of block
        uint16_t bytes_to_copy = (uint16_t) (BLOCK_SIZE - block_loc);
        //set bytes to read to = rest of block or bytes
        bytes_to_copy = bytes_to_copy <= (bytes_to_read-bytes_read) ? bytes_to_copy : (bytes_to_read-bytes_read);

        //copy disk to data
        memcpy(data+bytes_read, disk + get_disk_pos(disk, p_file->FAT_LOC, p_file->data_loc), bytes_to_copy);
        bytes_read += bytes_to_copy;
        p_file->data_loc += bytes_to_copy;
    }
    return bytes_read;
}

uint32_t get_disk_pos(void *disk, uint16_t fat_loc, uint16_t data_loc){
    uint16_t no_link = NO_LINK;
    uint16_t old_fat_loc = fat_loc;
    while(data_loc>=BLOCK_SIZE){
        fat_loc = fat_value(disk,fat_loc);
        if(fat_loc==NO_LINK){
            fat_loc = get_free_block(disk,0x0000);
            //write to fat
            uint32_t fat_on_disk_loc = fat_location(true, old_fat_loc);//get disk location of the old fat
            memcpy(disk+fat_on_disk_loc, &fat_loc, 2);//put the value of the new fat in its spot
            fat_on_disk_loc = fat_location(true, fat_loc);//get the disk location of the new fat
            memcpy(disk+fat_on_disk_loc, &no_link, 2);//put the no link in memory
            //do the same for fat2
            fat_on_disk_loc = fat_location(false, old_fat_loc);
            memcpy(disk+fat_on_disk_loc, &fat_loc, 2);
            fat_on_disk_loc = fat_location(true, fat_loc);
            memcpy(disk+fat_on_disk_loc, &no_link, 2);
        }
        old_fat_loc = fat_loc;
        data_loc-=BLOCK_SIZE;
    }
    return (uint32_t) (USER_SPACE_LOCATION + fat_loc * BLOCK_SIZE + data_loc);
}
/*
void get_dir_data(void* disk,MY_FILE *file,char data[ENTRY_SIZE]){
    MY_FILE p_file;
    p_file.isEOF=false;p_file.data_loc=0;p_file.FAT_LOC=file->pFAT_LOC;p_file.DATA_SIZE=MAX_SIZE;
    read_data(disk,&p_file,data,ENTRY_SIZE);
    uint16_t check_fat;
    memcpy(&check_fat,data+NAME_LENGTH+EXT_LENGTH, sizeof(uint16_t));
}*/


MY_FILE *create_file(void *disk,MY_FILE *parent, char name[NAME_LENGTH],char ext[EXT_LENGTH],
        char *data,uint16_t size){
    time_t the_time = time(NULL);
    uint16_t fat_loc = get_free_block(disk,0x0000);
    struct dir_entry entry = create_entry(name,ext,size,the_time,the_time,fat_loc);
    write_file_to_fat(entry,disk);

    //edit size in parent
    //special case if file is in root then the dir information is in the BOOT SECTOR
    uint32_t dir_disk_loc=ROOT_LOCATION;
    if(parent->FAT_LOC!=0) {
        uint16_t check_fat=0;
        uint16_t d_search =0;
        while(d_search<MAX_SIZE-ENTRY_SIZE && check_fat != parent->FAT_LOC) {
            uint32_t d_pos = get_disk_pos(disk, parent->pFAT_LOC, d_search);
            memcpy(&check_fat, disk + d_pos + ENTRY_SIZE - sizeof(uint16_t), sizeof(uint16_t));
            d_search+=ENTRY_SIZE;
        }
        printf("check fat %d real fat %d\n",check_fat,parent->FAT_LOC);
    }
    uint16_t d_size;
    memcpy(&d_size,disk+dir_disk_loc+NAME_LENGTH+EXT_LENGTH, sizeof(uint16_t));
    d_size+=ENTRY_SIZE;
    memcpy(disk+dir_disk_loc+NAME_LENGTH+EXT_LENGTH,&d_size,sizeof(uint16_t));

    //add file info to the directory
    parent->data_loc=parent->DATA_SIZE;
    char dir_entry_data[ENTRY_SIZE];
    entry_to_data(entry,dir_entry_data);
    write_data(disk,parent,dir_entry_data,ENTRY_SIZE);


    //init the file pointer*/
    MY_FILE *my_file=malloc(sizeof(MY_FILE));
    my_file->data_loc = 0;
    my_file->FAT_LOC = fat_loc;
    my_file->DATA_SIZE=size;
    my_file->isEOF =false;
    my_file->pFAT_LOC=parent->FAT_LOC;
    //write_data
    write_data(disk, my_file, data, size);
    my_file->data_loc = 0;
    return my_file;
}

uint16_t block_pos(void *disk,uint16_t fat_loc,uint16_t data_loc){
    uint32_t user_loc = get_disk_pos(disk, fat_loc, data_loc)-USER_SPACE_LOCATION;//location in the user space
    return (uint16_t) (user_loc % BLOCK_SIZE); //location in specific block
}

void entry_to_data(struct dir_entry entry,char array[ENTRY_SIZE]){
    /*char *null_str = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
    memcpy(array,null_str,ENTRY_SIZE);*/

    memcpy(array,entry.name,strlen(entry.name));
    array+=NAME_LENGTH;

    memcpy(array,entry.extension,strlen(entry.extension));
    array+=EXT_LENGTH;

    memcpy(array,&entry.size, sizeof(entry.size));
    array+=sizeof(entry.size);

    memcpy(array,&entry.create_time, sizeof(entry.create_time));
    array+=sizeof(entry.create_time);

    memcpy(array,&entry.mod_time, sizeof(entry.mod_time));
    array+=sizeof(entry.mod_time);

    memcpy(array,&entry.FAT_location, sizeof(entry.FAT_location));
}


uint16_t write_data(void *disk, struct MY_FILE *p_file, void *data, uint16_t bytes){
    uint16_t bytes_wrote = 0;
    while(bytes_wrote<bytes) {
        //get location of data in block
        uint16_t block_loc = (uint16_t) (p_file->data_loc % BLOCK_SIZE);//uint16_t block_loc = block_pos(disk,p_file->FAT_LOC,p_file->data_loc);
        //set bytes to write to rest of block
        uint16_t bytes_to_copy = (uint16_t) (BLOCK_SIZE - block_loc);
        //set bytes what ever is smaller the remaining bytes or the rest of the block
        bytes_to_copy = bytes_to_copy <= (bytes-bytes_wrote) ? bytes_to_copy : (bytes-bytes_wrote);

        //copy disk to data
        memcpy(disk + get_disk_pos(disk, p_file->FAT_LOC, p_file->data_loc),data+bytes_wrote,bytes_to_copy);
        bytes_wrote += bytes_to_copy;
        p_file->data_loc += bytes_to_copy;
        p_file->DATA_SIZE = p_file->DATA_SIZE >= p_file->data_loc ? p_file->DATA_SIZE : p_file->data_loc;
    }
    return bytes_wrote;
}

void write_file_to_fat(struct dir_entry entry,void *disk){
    uint16_t no_link = NO_LINK;
    uint32_t p_loc = fat_location(true, entry.FAT_location);//get address from loc
    memcpy(disk+p_loc, &no_link, 2);//copy no link to this address
    //do the same for fat2
    p_loc = fat_location(false, entry.FAT_location);
    memcpy(disk+p_loc, &no_link, 2);
}

uint16_t get_free_block(void *disk, uint16_t start) {
    int blocks = 1;
    while(blocks<TOTAL_BLOCKS) {
        blocks++;
        start = (uint16_t) ((start + 1) % TOTAL_BLOCKS);
        uint16_t test = fat_value(disk,start);
        if(test==FREE_BLOCK){
            return start;
        }
    }
    exit(1);
}


void write_dir_entry(struct dir_entry entry,void *disk,uint32_t location){
    char *null_str = "\0\0\0\0\0\0\0\0\0";

    void *p_loc = disk+location;
    memcpy(p_loc,null_str,NAME_LENGTH);
    memcpy(p_loc,entry.name,strlen(entry.name));

    p_loc+=NAME_LENGTH;
    memcpy(p_loc,null_str,EXT_LENGTH);
    memcpy(p_loc,entry.extension,strlen(entry.extension));

    p_loc+=EXT_LENGTH;
    memcpy(p_loc,&entry.size, sizeof(entry.size));

    p_loc+=sizeof(entry.size);
    memcpy(p_loc,&entry.create_time, sizeof(entry.create_time));

    p_loc+=sizeof(entry.create_time);
    memcpy(p_loc,&entry.mod_time, sizeof(entry.mod_time));

    p_loc+=sizeof(entry.mod_time);
    memcpy(p_loc,&entry.FAT_location, sizeof(entry.FAT_location));
}
uint32_t fat_location(bool isFAT1,uint16_t block){
    uint32_t FAT_loc = isFAT1 ? FAT1_LOCATION : FAT2_LOCATION ;
    return FAT_loc + block* sizeof(uint16_t);
}

uint16_t fat_value(void* disk, uint16_t block){
    uint p_loc = fat_location(true,block);
    uint16_t return_value;
    memcpy(&return_value,disk+p_loc,2);
    return return_value;
}

struct dir_entry create_root(){
    return create_entry("root\0","\\\\\\",0,time(NULL),time(NULL),0);
}

struct dir_entry create_entry(char name[NAME_LENGTH],char extension[EXT_LENGTH],uint16_t size,
        time_t create_time, time_t mod_time,uint16_t FAT_location){
    struct dir_entry entry;
    strcpy(entry.name,name);
    strcpy(entry.extension,extension);
    entry.size = size;
    entry.create_time = create_time;
    entry.mod_time = mod_time;
    entry.FAT_location = FAT_location;
    return entry;
}


struct boot create_boot() {
    struct boot b;
    strcpy(b.valid_check,"This is Adam's FAT Drive");
    b.boot_sector_size = BOOT_SECTOR_SIZE;
    b.total_size = TOTAL_SIZE;
    b.block_size = BLOCK_SIZE;
    b.total_blocks = TOTAL_BLOCKS;
    b.FAT_size = FAT_SIZE;
    b.max_size = MAX_SIZE;
    b.root = create_root();
    return b;
}