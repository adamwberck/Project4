//
// Created by mandr on 2019-04-17.
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
#include "my_stack.h"

#define EMPTY   0x0000
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


struct directory_entry{
    char name[NAME_LENGTH];
    char extension[EXT_LENGTH];
    unsigned short int size;
    time_t create_time;
    time_t mod_time;
    unsigned short int FAT_location;
};

struct boot{
    char valid_check[24];
    unsigned short int boot_sector_size; //512
    unsigned int total_size; //2096128
    unsigned short int block_size; //512
    unsigned short int total_blocks; //4094
    unsigned short int FAT_size; //8188
    unsigned short int max_size; //65535
    struct directory_entry root;
};


void write_file_to_fat(struct directory_entry entry,void *data);
void write_dir_entry(struct directory_entry entry,void *data,unsigned int location);
void *fat_location(bool isFAT1, void* data,int block);
struct directory_entry create_root();
struct boot create_boot();
struct directory_entry create_entry(char name[9],char extension[3],unsigned short int size, time_t create_time,
        time_t mod_time,unsigned short int FAT_location);

void print_mapped_data(unsigned char *mapped_data);

unsigned short get_free_block(void *data,unsigned short int start);

int main(){
    FILE* disk = fopen("my_disk","w+");
    unsigned short int empty = EMPTY;
    for(int i=0;i<TOTAL_SIZE;i++) {
        fwrite(&empty, sizeof(unsigned short int), 1, disk);
    }
    fseek(disk,0,SEEK_SET);
    struct boot my_boot = create_boot();
    fwrite(my_boot.valid_check,1, sizeof(my_boot.valid_check),disk);
    fwrite(&my_boot.boot_sector_size,2, 1,disk);
    fwrite(&my_boot.total_size,4, 1,disk);
    fwrite(&my_boot.block_size,2, 1,disk);
    fwrite(&my_boot.total_blocks,2, 1,disk);
    fwrite(&my_boot.FAT_size,2, 1,disk);
    fwrite(&my_boot.max_size,2, 1,disk);
    //write_dir_entry(my_boot.root,disk);
    fclose(disk);

    int i_disk = open("my_disk",O_RDWR,0);
    void *mapped_data = mmap(NULL,TOTAL_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,i_disk,0);
    write_dir_entry(my_boot.root,mapped_data,ROOT_LOCATION);
    write_file_to_fat(my_boot.root,mapped_data);

    void *p = mapped_data+FAT2_LOCATION;
    unsigned short d = 0xABCD;
    memcpy(p,&d,2);
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

void write_file_to_fat(struct directory_entry entry,void *data){
    int blocks = (entry.size/BLOCK_SIZE);
    //add a block if there is more data left over or if size is zero
    blocks+= entry.size % BLOCK_SIZE == 0 && blocks>0 ? 0 : 1;

    struct my_stack stack = create_my_stack();//create stack to add empty blocks
    put_my_stack(&stack,entry.FAT_location);//put block already allocated when adding to directory
    //add extra blocks
    unsigned short int free = entry.FAT_location;
    for(int i=1;i<blocks;i++){
        free = get_free_block(data,free);
        put_my_stack(&stack,free);
    }
    //set the fat using the stack
    unsigned short int last_loc = NO_LINK;//add no link

    while(stack.size>0) {
        unsigned short loc = pop_my_stack(&stack);//get last added block
        void *p_loc;

        p_loc = fat_location(true, data, loc);//get address from loc
        memcpy(p_loc, &last_loc, 2);//copy last loc to this address
        //do the same for fat2
        //p_loc = fat_location(false, data, loc);
        //memcpy(p_loc, &last_loc, 2);

        last_loc = loc;//reset fat
    }
}

unsigned short get_free_block(void *data, unsigned short start) {
    void *p_loc = data+FAT1_LOCATION;
    start++;
    start*=2;
    //for(unsigned short i = start;i<TOTAL_BLOCKS*2;i+=2){
    unsigned short i = start;
    if(i>=TOTAL_BLOCKS*2){
        i=0;
    }
    int blocks = 1;
    while(blocks<TOTAL_BLOCKS){
        blocks++;
        p_loc+=i;
        unsigned short test;
        memcpy(&test,p_loc,2);
        if(test==EMPTY){
            return (unsigned short) (i / 2);
        }

        i+=2;
    }

    exit(1);
}


void write_dir_entry(struct directory_entry entry,void *data,unsigned int location){
    void *p_loc = data+location;
    memcpy(p_loc,entry.name,NAME_LENGTH);

    p_loc+=NAME_LENGTH;
    memcpy(p_loc,entry.extension,EXT_LENGTH);

    p_loc+=EXT_LENGTH;
    memcpy(p_loc,&entry.size, sizeof(entry.size));

    p_loc+=sizeof(entry.size);
    memcpy(p_loc,&entry.create_time, sizeof(entry.create_time));

    p_loc+=sizeof(entry.create_time);
    memcpy(p_loc,&entry.mod_time, sizeof(entry.mod_time));

    p_loc+=sizeof(entry.mod_time);
    memcpy(p_loc,&entry.FAT_location, sizeof(entry.FAT_location));
}
void *fat_location(bool isFAT1,void* data, int block){
    int FAT_loc = isFAT1 ? FAT1_LOCATION : FAT2_LOCATION ;
    return data+FAT_loc + block*2;
}

struct directory_entry create_root(){
    return create_entry("root\0","\\\\\\",0,time(NULL),time(NULL),0x0000);
}

struct directory_entry create_entry(char name[NAME_LENGTH],char extension[EXT_LENGTH],unsigned short int size,
        time_t create_time, time_t mod_time,unsigned short int FAT_location){
    struct directory_entry entry;
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