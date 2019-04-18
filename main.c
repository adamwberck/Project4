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

struct MY_FILE{
    uint32_t disk_loc;
    uint16_t data_loc;
    uint16_t data_size;
    uint16_t fat_loc;
    bool isEOF;
};


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


void write_data(void *disk,char *data,uint16_t size,uint16_t fat_loc);
void create_file(void *disk,struct dir_entry parent, char name[NAME_LENGTH],char ext[EXT_LENGTH],
                 char *data,uint16_t size);
void write_file_to_fat(struct dir_entry entry,void *disk);
void write_dir_entry(struct dir_entry entry,void *disk,uint32_t location);
uint16_t fat_value(void* disk, uint16_t block);
void *fat_location(bool isFAT1, void* data,uint16_t block);
struct dir_entry create_root();
struct boot create_boot();
struct dir_entry create_entry(char name[9],char extension[3],uint16_t size, time_t create_time,
        time_t mod_time,uint16_t FAT_location);

void print_mapped_data(unsigned char *mapped_data);

uint16_t get_free_block(void *disk,uint16_t start);

int main(){
    FILE* new_disk = fopen("my_disk","w+");
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

    int i_disk = open("my_disk",O_RDWR,0);
    void *disk = mmap(NULL,TOTAL_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,i_disk,0);
    write_dir_entry(my_boot.root,disk,ROOT_LOCATION);
    write_file_to_fat(my_boot.root,disk);

    char *data = "Hello this is a test file. The quick fox jumped over the lazy brown dog. Iron man is the first, "
                 "then the Hulk, then Thor, and finally Captain America."
                 "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nunc neque tortor, eleifend ut nisi vel,"
                 " porttitor facilisis sem. Mauris consectetur purus ex, a condimentum leo consectetur sit amet."
                 " Pellentesque at feugiat dolor. Ut accumsan lectus quis nulla vestibulum ornare."
                 " Curabitur ultrices pulvinar nisl, ac aliquet nibh venenatis sed."
                 " Phasellus lobortis et felis non semper. Vivamus eu dolor ut tellus lacinia viverra."
                 " Donec egestas, ligula eget mattis scelerisque, lacus ante consectetur erat sed.\0";
    create_file(disk, my_boot.root, "test\0", "txt", data, (uint16_t) strlen(data));


    //void *p = disk+USER_SPACE_LOCATION;
    //uint16_t d = 0xABCD;
    //memcpy(p,&d,2);
    //print_mapped_data(mapped_data);
}

void print_mapped_data(unsigned char *mapped_data) {
    for(int i=FAT1_LOCATION; i < FAT1_LOCATION+16; i++){
        if(i%16==0){
            printf("\n");
        }
        printf("%02x ",mapped_data[i]);
    }
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

size_t seek_data(void *disk,struct MY_FILE *p_file, uint16_t offset, int whence){
    uint32_t user_loc = p_file->disk_loc-USER_SPACE_LOCATION; //location in the user space
    uint16_t block_loc = (uint16_t) (user_loc % BLOCK_SIZE); //location in specific block
    uint16_t fat_loc  = (uint16_t) (user_loc / BLOCK_SIZE);

}

size_t read_data(void *disk,struct MY_FILE *p_file,void *data, uint16_t bytes){
    uint16_t bytes_read = 0;
    uint32_t user_loc = p_file->disk_loc-USER_SPACE_LOCATION; //location in the user space
    uint16_t block_loc = (uint16_t) (user_loc % BLOCK_SIZE); //location in specific block
    uint16_t remaining_bytes = p_file->data_size-p_file->data_loc; //amount of bytes left in file

    p_file->isEOF = bytes>=remaining_bytes; //if remaining bytes is less than bytes available set EOF true
    uint16_t  bytes_to_read = bytes<=remaining_bytes ? bytes : remaining_bytes; //only get bytes <= to bytes available
    //set bytes to read to rest of block
    uint16_t bytes_to_copy = (uint16_t) (BLOCK_SIZE - block_loc);
    //set bytes to read to = rest of block or bytes
    bytes_to_copy = bytes_to_copy <= bytes_to_read ? bytes_to_copy : bytes_to_read;
    //copy disk to data
    memcpy(data, disk + p_file->disk_loc, bytes_to_copy);
    bytes_read+=bytes_to_copy;
    p_file->data_loc+=bytes_to_copy;
    //seek()
}


void create_file(void *disk,struct dir_entry parent, char name[NAME_LENGTH],char ext[EXT_LENGTH],
        char *data,uint16_t size){
    time_t the_time = time(NULL);
    uint16_t fat_loc = get_free_block(disk,0x0000);
    struct dir_entry entry = create_entry(name,ext,size,the_time,the_time,fat_loc);
    uint32_t disk_loc = get_dir_location(disk,parent);
    //printf("disk_loc %x\n",disk_loc);
    write_dir_entry(entry,disk,disk_loc);
    write_file_to_fat(entry,disk);
    write_data(disk,data,size,fat_loc);
}

void write_data(void *disk,char *data,uint16_t size,uint16_t fat_loc){
    char block_of_data[BLOCK_SIZE];
    int temp_size = size;
    int disk_loc = USER_SPACE_LOCATION+fat_loc*BLOCK_SIZE;
    while(temp_size>0){
        memcpy(block_of_data,data,BLOCK_SIZE);
        memcpy(disk+disk_loc,block_of_data,BLOCK_SIZE);

        fat_loc=fat_value(disk,fat_loc);
        disk_loc = USER_SPACE_LOCATION+fat_loc*BLOCK_SIZE;
        data += BLOCK_SIZE;
        temp_size -= BLOCK_SIZE;
    }
}

void write_file_to_fat(struct dir_entry entry,void *disk){
    int blocks = (entry.size/BLOCK_SIZE);
    //add a block if there is more disk left over or if size is zero
    blocks+= entry.size % BLOCK_SIZE == 0 && blocks>0 ? 0 : 1;

    struct my_stack stack = create_my_stack();//create stack to add empty blocks
    put_my_stack(&stack,entry.FAT_location);//put block already allocated when adding to directory
    //add extra blocks
    uint16_t free = entry.FAT_location;
    for(int i=1;i<blocks;i++){
        free = get_free_block(disk,free);
        put_my_stack(&stack,free);
    }
    //set the fat using the stack
    uint16_t last_loc = NO_LINK;//add no link

    while(stack.size>0) {
        uint16_t loc = pop_my_stack(&stack);//get last added block
        void *p_loc;

        //write to fat
        p_loc = fat_location(true, disk, loc);//get address from loc
        memcpy(p_loc, &last_loc, 2);//copy last loc to this address
        //do the same for fat2
        p_loc = fat_location(false, disk, loc);
        memcpy(p_loc, &last_loc, 2);

        last_loc = loc;//reset fat
    }
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
void *fat_location(bool isFAT1,void* data, uint16_t block){
    int FAT_loc = isFAT1 ? FAT1_LOCATION : FAT2_LOCATION ;
    return data+FAT_loc + block*2;
}

uint16_t fat_value(void* disk, uint16_t block){
    void *p_loc = fat_location(true, disk, block);
    uint16_t return_value;
    memcpy(&return_value,p_loc,2);
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