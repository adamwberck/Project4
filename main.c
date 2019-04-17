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

#define EMPTY   0x0000
#define NO_LINK 0xFFFF

#define BOOT_SECTOR_SIZE 512
#define TOTAL_SIZE 2096128
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

struct directory_entry{
    char name[9];
    char extension[3];
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

void write_dir_entry(struct directory_entry entry,FILE *disk);
struct directory_entry create_root();
struct boot create_boot();
struct directory_entry create_entry(char name[9],char extension[3],unsigned short int size, time_t create_time,
        time_t mod_time,unsigned short int FAT_location);

void print_mapped_data(unsigned char *mapped_data);

int main(){
    printf("%zu\n", sizeof(char));

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
    write_dir_entry(my_boot.root,disk);
    fclose(disk);
    int i_disk = open("my_disk",O_RDWR,0);
    unsigned char *mapped_data = mmap(NULL,TOTAL_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,i_disk,0);

    unsigned char loc[2]; loc[0] = NO_LINK & 0xFF; loc[1] = NO_LINK>>8;
    mapped_data[FAT1_LOCATION+my_boot.root.FAT_location]   =  loc[0];
    mapped_data[FAT1_LOCATION+my_boot.root.FAT_location+1] =  loc[1];
    mapped_data[FAT2_LOCATION+my_boot.root.FAT_location]   =  loc[0];
    mapped_data[FAT2_LOCATION+my_boot.root.FAT_location+1] =  loc[1];



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

//not correct write now
void write_dir_entry(struct directory_entry entry,FILE *disk){
    fwrite(entry.name,1, sizeof(entry.name),disk);
    fwrite(entry.extension,1, sizeof(entry.extension),disk);
    fwrite(&entry.size,2, 1,disk);
    fwrite(&entry.create_time,8, 1,disk);
    fwrite(&entry.mod_time,8, 1,disk);
    fwrite(&entry.FAT_location,2, 1,disk);
}


struct directory_entry create_root(){
    return create_entry("root\0","\\\\\\",0,time(NULL),time(NULL),0x0000);
}

struct directory_entry create_entry(char name[9],char extension[3],unsigned short int size, time_t create_time,
                         time_t mod_time,unsigned short int FAT_location){
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