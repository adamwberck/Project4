//
// Created by Adam Berck on 2019-04-17.
//

#ifndef PROJECT4_MAIN_H
#define PROJECT4_MAIN_H
#include <stdio.h>
#include <memory.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>


#define FREE_BLOCK   0x0000
#define NO_LINK 0xFFFF

#define BOOT_SECTOR_SIZE 512
#define TOTAL_SIZE 2114048
#define BLOCK_SIZE 512
#define FAT_SIZE 8192
#define MAX_SIZE 65535

//entry is 32 bytes
#define ENTRY_SIZE 32

//root location is 0x26
#define ROOT_LOCATION 0x26
#define FAT1_LOCATION 0x200
#define FAT2_LOCATION 0x2200
#define USER_SPACE_LOCATION 0x4200

#define NAME_LENGTH 9
#define EXT_LENGTH 3
#define TOTAL_BLOCKS 4096



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

void display_everything();
void display_file(struct dir_entry entry,MY_FILE *file,int depth);
void root_to_myfile();
void close_file(MY_FILE *file);
void create_disk(struct boot my_boot);
MY_FILE *open_file(MY_FILE *parent,char name[NAME_LENGTH],char ext[EXT_LENGTH]);
MY_FILE *move_file(MY_FILE *new_folder, MY_FILE *parent,MY_FILE *file,char name[NAME_LENGTH],
        char ext[EXT_LENGTH]);
MY_FILE *copy_file(MY_FILE *new_folder,MY_FILE *file,char name[NAME_LENGTH],char ext[EXT_LENGTH]);
bool seek_to_dir_entry(struct dir_entry *entry,MY_FILE *parent, const char *filename, const char *ext);
void entry_to_myfile(const MY_FILE *parent, struct dir_entry *entry, MY_FILE *dir_file);
MY_FILE *user_create_file(MY_FILE *parent,char *name,char *ext,char *data,uint16_t size);
void entry_to_data(struct dir_entry entry,char array[ENTRY_SIZE]);
void delete_file(MY_FILE *parent, char filename[NAME_LENGTH],char ext[EXT_LENGTH] );
uint16_t write_data( struct MY_FILE *p_file, void *data, uint16_t bytes);
uint32_t get_disk_pos( uint16_t fat_loc, uint16_t data_loc);
MY_FILE *create_file(MY_FILE *parent, char name[NAME_LENGTH],char ext[EXT_LENGTH], char *data,uint16_t size);
void write_file_to_fat(struct dir_entry entry,void *disk);
void write_dir_entry(struct dir_entry entry,uint32_t location);
uint16_t fat_value( uint16_t block);
uint32_t fat_location(bool isFAT1, uint16_t block);
struct dir_entry create_root();
struct boot create_boot();
struct dir_entry create_entry(char name[9],char extension[3],uint16_t size, time_t create_time,
        time_t mod_time,uint16_t FAT_location);
MY_FILE *make_dir(MY_FILE *parent,char *name);
uint16_t get_free_block(uint16_t start);
uint16_t read_data(struct MY_FILE *p_file,void *data, uint16_t bytes);
off_t fsize(const char *filename);
void erase_fat( uint16_t fat_loc);
void data_to_entry(char data[32], struct dir_entry *new_entry);


#endif //PROJECT4_MAIN_H
