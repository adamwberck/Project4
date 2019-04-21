//
// Created by Adam Berck on 2019-04-17.
//
#include "disk.h"
#include "my_stack.h"
#include "my_dir_stack.h"
#include "test.h"

void *disk;
MY_FILE *current_dir;
struct my_dir_stack dir_stack;
void root_my_file();
const char *FOLDER_EXT = "\\\\\\";
int main(){
    dir_stack = create_my_dir_stack();
    struct boot my_boot = create_boot();
    create_disk(my_boot);

    //using mmap to open my_disk
    int int_disk = open("my_disk",O_RDWR,0);
    disk = mmap(NULL,TOTAL_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,int_disk,0);
    write_dir_entry(my_boot.root,ROOT_LOCATION);
    write_file_to_fat(my_boot.root,disk);

    MY_FILE *root_folder = malloc(sizeof(MY_FILE));
    root_my_file(my_boot,root_folder);
    current_dir = root_folder;
    first_test(my_boot);
}

void change_dir(char name[NAME_LENGTH]){
    if(strcmp(name,"..")==0 && dir_stack.size>0){
        current_dir=pop_my_dir_stack(&dir_stack);
        free(current_dir);
        return;
    }
    struct dir_entry entry;
    if(seek_to_dir_entry(&entry,current_dir,name,FOLDER_EXT)){
        MY_FILE *new_dir = malloc(sizeof(MY_FILE));
        entry_to_myfile(current_dir,&entry,new_dir);
        put_my_dir_stack(&dir_stack, current_dir);
        free(current_dir);
        current_dir = new_dir;
    }
    printf("Couldn't find directory %s\n",name);
}

void root_my_file(struct boot my_boot,MY_FILE root_folder) {
    root_folder.FAT_LOC=my_boot.root.FAT_location;
    root_folder.isEOF=false;
    root_folder.data_loc=0;
    root_folder.DATA_SIZE=my_boot.root.size;
}

void create_disk(struct boot my_boot){
    FILE *new_disk = fopen("my_disk","w+");
    uint16_t empty = FREE_BLOCK;
    for(int i=0;i<TOTAL_SIZE;i++) {
        fwrite(&empty, sizeof(uint16_t), 1, new_disk);
    }
    fseek(new_disk,0,SEEK_SET);
    fwrite(my_boot.valid_check,1, sizeof(my_boot.valid_check),new_disk);
    fwrite(&my_boot.boot_sector_size,2, 1,new_disk);
    fwrite(&my_boot.total_size,4, 1,new_disk);
    fwrite(&my_boot.block_size,2, 1,new_disk);
    fwrite(&my_boot.total_blocks,2, 1,new_disk);
    fwrite(&my_boot.FAT_size,2, 1,new_disk);
    fwrite(&my_boot.max_size,2, 1,new_disk);
    fclose(new_disk);
}



MY_FILE *move_file(MY_FILE *new_folder, MY_FILE *parent,MY_FILE *file,char name[NAME_LENGTH],char ext[EXT_LENGTH]){
    MY_FILE *new_file = copy_file(new_folder,file,name,ext);
    delete_file(parent,name,ext);
    return new_file;
}

MY_FILE *open_file(MY_FILE *parent,char name[NAME_LENGTH],char ext[EXT_LENGTH]){
    struct dir_entry entry;
    parent->isEOF=false;
    parent->data_loc=0;
    if(!seek_to_dir_entry(&entry,parent,name,ext)){
        printf("Couldn't find %s.%s\n",name,ext);
        return NULL;
    }
    parent->isEOF=false;
    parent->data_loc=0;
    MY_FILE *file = malloc(sizeof(MY_FILE));
    entry_to_myfile(parent,&entry,file);
    return file;
}

void close_file(MY_FILE *file) {
    free(file);
}

MY_FILE *copy_file(MY_FILE *new_folder,MY_FILE *file,char name[NAME_LENGTH],char ext[EXT_LENGTH]){
    char data[file->DATA_SIZE];
    read_data(file,data,file->DATA_SIZE);
    MY_FILE *copied_file = create_file(new_folder,name,ext,data,file->DATA_SIZE);
    //if you are copying a folder
    if(strcmp(ext,"\\\\\\")==0){
        //read the data from the copied file
        char copy_data[ENTRY_SIZE];
        read_data(file,copy_data,ENTRY_SIZE);
        struct dir_entry copy_entry;
        data_to_entry(copy_data,&copy_entry);
        MY_FILE sub_file_copy;
        entry_to_myfile(copied_file,&copy_entry,&sub_file_copy);
        //recursive copy the files that are in the folder
        copy_file(copied_file,&sub_file_copy,copy_entry.name,copy_entry.extension);
    }
    return copied_file;
}

void delete_file(MY_FILE *parent, char *filename, char *ext ){
    //add file info to the directory
    parent->data_loc=0;
    //get dir entry
    struct dir_entry entry;
    if(!seek_to_dir_entry(&entry,parent,filename,ext)){
        printf("Couldn't find %s.%s\n",filename,ext);
        return;
    }

    //edit size in parent
    //special case if file is in root then the dir information is in the BOOT SECTOR
    uint32_t dir_disk_loc=ROOT_LOCATION;
    if(parent->FAT_LOC!=0) {
        uint16_t check_fat=0;
        uint16_t d_search =0;
        while(d_search<MAX_SIZE-ENTRY_SIZE && check_fat != parent->FAT_LOC) {
            dir_disk_loc = get_disk_pos(parent->pFAT_LOC, d_search);
            memcpy(&check_fat, disk + dir_disk_loc + ENTRY_SIZE - sizeof(uint16_t), sizeof(uint16_t));
            d_search+=ENTRY_SIZE;
        }
    }

    //BAD if goes across a block
    uint16_t d_size;
    memcpy(&d_size,disk+dir_disk_loc+NAME_LENGTH+EXT_LENGTH, sizeof(uint16_t));
    parent->DATA_SIZE=d_size;
    d_size-=ENTRY_SIZE;
    memcpy(disk+dir_disk_loc+NAME_LENGTH+EXT_LENGTH,&d_size,sizeof(uint16_t));

    //check if its a folder
    if(memcmp(entry.extension,"\\\\\\",3)==0){
        //if its a folder clear all the fat of the sub files
        MY_FILE dir_file;
        entry_to_myfile(parent, &entry, &dir_file);
        while(!dir_file.isEOF) {
            char interior_entry_data[ENTRY_SIZE];
            read_data(&dir_file, interior_entry_data, ENTRY_SIZE);
            struct dir_entry interior_entry;
            data_to_entry(interior_entry_data,&interior_entry);
            //recursive delete the interior files
            uint16_t old_pos = dir_file.data_loc;
            bool old_isEOF = dir_file.isEOF;
            dir_file.data_loc=0;
            dir_file.isEOF=false;
            delete_file(&dir_file,interior_entry.name,interior_entry.extension);
            dir_file.isEOF=old_isEOF;
            dir_file.data_loc=old_pos;
        }
    }

    //erase fat
    erase_fat( entry.FAT_location);

    //erase dir info
    parent->data_loc+=ENTRY_SIZE;
    uint16_t old_data_loc = parent->data_loc;
    uint16_t amount_of_data = parent->DATA_SIZE-old_data_loc;
    char rest_of_data[amount_of_data];
    read_data(parent,rest_of_data,amount_of_data);
    parent->data_loc = (uint16_t) (old_data_loc - ENTRY_SIZE);
    write_data(parent,rest_of_data,amount_of_data);
}

void entry_to_myfile(const MY_FILE *parent, struct dir_entry *entry, MY_FILE *dir_file) {
    dir_file->data_loc=0;
    dir_file->DATA_SIZE= (*entry).size;
    dir_file->isEOF=false;
    dir_file->FAT_LOC= (*entry).FAT_location;
    dir_file->pFAT_LOC=parent->FAT_LOC;
}

//returns true if found false if it doesn't exist in parent
bool seek_to_dir_entry(struct dir_entry *entry,MY_FILE *parent, const char *filename, const char *ext){
    bool n = false;
    bool e = false;
    char raw_entry_data[ENTRY_SIZE];
    while(!parent->isEOF && !(n && e)) {
        read_data(parent,raw_entry_data,ENTRY_SIZE);
        data_to_entry(raw_entry_data, entry);
        int name_len = (int) fmin(strlen(filename), NAME_LENGTH);
        n = memcmp(filename, entry->name, name_len) == 0;
        int ext_len = (int) fmin(strlen(ext), EXT_LENGTH);
        e = memcmp(ext, entry->extension, ext_len) == 0;
    }
    parent->data_loc-=ENTRY_SIZE;
    return n&&e;
}

void data_to_entry(char data[32], struct dir_entry *new_entry) {
    memcpy(new_entry->name,data,NAME_LENGTH);
    data+=NAME_LENGTH;

    memcpy(new_entry->extension,data,EXT_LENGTH);
    data+=EXT_LENGTH;

    memcpy(&new_entry->size,data,sizeof(int16_t));
    data+=sizeof(int16_t);
    memcpy(&new_entry->create_time,data,sizeof(time_t));
    data+=sizeof(time_t);
    memcpy(&new_entry->mod_time,data,sizeof(time_t));
    data+=sizeof(time_t);
    memcpy(&new_entry->FAT_location,data, sizeof(int16_t));
}

void erase_fat(uint16_t fat_loc) {
    struct my_stack stack = create_my_stack();
    put_my_stack(&stack,fat_loc);
    while(fat_loc!=NO_LINK){
        fat_loc = fat_value(fat_loc);
        put_my_stack(&stack,fat_loc);
    }
    uint16_t free_block = FREE_BLOCK;
    while(stack.size>0){
        //erase the first fat
        uint16_t fat_to_erase = pop_my_stack(&stack);
        uint32_t f_pos = fat_location(true,fat_to_erase);
        memcpy(disk+f_pos,&free_block, sizeof(uint16_t));
        //erase the second fat
        f_pos = fat_location(false,fat_to_erase);
        memcpy(disk+f_pos,&free_block, sizeof(uint16_t));
    }
}


off_t fsize(const char *filename) {
    struct stat st;

    if (stat(filename, &st) == 0)
        return st.st_size;

    return -1;
}

uint32_t get_dir_location(struct dir_entry parent){
    int temp_size = parent.size;
    uint16_t fat = parent.FAT_location;
    while(temp_size>BLOCK_SIZE){
        fat = fat_value(fat);
        temp_size-=BLOCK_SIZE;
    }
    return (uint32_t) (USER_SPACE_LOCATION + fat * BLOCK_SIZE + temp_size);
}

uint16_t seek_data(struct MY_FILE *p_file, uint16_t offset, int whence){
    /*
    uint32_t user_loc = p_file->disk_pos-USER_SPACE_LOCATION; //location in the user space
    uint16_t block_loc = (uint16_t) (user_loc % BLOCK_SIZE); //location in specific block
    uint16_t fat_loc  = (uint16_t) (user_loc / BLOCK_SIZE);*/
}

uint16_t read_data(struct MY_FILE *p_file,void *data, uint16_t bytes){
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
        memcpy(data+bytes_read, disk + get_disk_pos( p_file->FAT_LOC, p_file->data_loc), bytes_to_copy);
        bytes_read += bytes_to_copy;
        p_file->data_loc += bytes_to_copy;
    }
    return bytes_read;
}

uint32_t get_disk_pos( uint16_t fat_loc, uint16_t data_loc){
    uint16_t no_link = NO_LINK;
    uint16_t old_fat_loc = fat_loc;
    while(data_loc>=BLOCK_SIZE){
        fat_loc = fat_value(fat_loc);
        if(fat_loc==NO_LINK){
            fat_loc = get_free_block(0x0000);
            //write to fat
            uint32_t fat_on_disk_loc = fat_location(true, old_fat_loc);//get disk location of the old fat
            memcpy(disk+fat_on_disk_loc, &fat_loc, 2);//put the value of the new fat in its spot
            fat_on_disk_loc = fat_location(true, fat_loc);//get the disk location of the new fat
            memcpy(disk+fat_on_disk_loc, &no_link, 2);//put the no link in memory
            //do the same for fat2
            fat_on_disk_loc = fat_location(false, old_fat_loc);
            memcpy(disk+fat_on_disk_loc, &fat_loc, 2);
            fat_on_disk_loc = fat_location(false, fat_loc);
            memcpy(disk+fat_on_disk_loc, &no_link, 2);
        }
        old_fat_loc = fat_loc;
        data_loc-=BLOCK_SIZE;
    }
    return (uint32_t) (USER_SPACE_LOCATION + fat_loc * BLOCK_SIZE + data_loc);
}

MY_FILE *create_file(MY_FILE *parent, char name[NAME_LENGTH],char ext[EXT_LENGTH],char *data,uint16_t size){
    if(strlen(name)<=0){
        return NULL;
    }
    struct dir_entry dup_file;
    //forbid the creation of a duplicate file
    if(seek_to_dir_entry(&dup_file,parent,name,ext)){
        return NULL;
    }
    //reset the pointer
    parent->data_loc=0;
    parent->isEOF=false;

    time_t the_time = time(NULL);
    uint16_t fat_loc = get_free_block(0x0000);
    struct dir_entry entry = create_entry(name,ext,size,the_time,the_time,fat_loc);
    write_file_to_fat(entry,disk);

    //edit size in parent
    //special case if file is in root then the dir information is in the BOOT SECTOR
    uint32_t dir_disk_loc=ROOT_LOCATION;
    if(parent->FAT_LOC!=0) {
        uint16_t check_fat=0;
        uint16_t d_search =0;
        while(d_search<MAX_SIZE-ENTRY_SIZE && check_fat != parent->FAT_LOC) {
            dir_disk_loc = get_disk_pos(parent->pFAT_LOC, d_search);
            memcpy(&check_fat, disk + dir_disk_loc + ENTRY_SIZE - sizeof(uint16_t), sizeof(uint16_t));
            d_search+=ENTRY_SIZE;
        }
    }
    uint16_t d_size;
    memcpy(&d_size,disk+dir_disk_loc+NAME_LENGTH+EXT_LENGTH, sizeof(uint16_t));
    d_size+=ENTRY_SIZE;
    memcpy(disk+dir_disk_loc+NAME_LENGTH+EXT_LENGTH,&d_size,sizeof(uint16_t));

    //add file info to the directory
    parent->DATA_SIZE= (uint16_t) (d_size - ENTRY_SIZE);
    parent->data_loc=parent->DATA_SIZE;
    char dir_entry_data[ENTRY_SIZE];
    entry_to_data(entry,dir_entry_data);
    write_data(parent,dir_entry_data,ENTRY_SIZE);
    parent->DATA_SIZE+=ENTRY_SIZE;

    //init the file pointer*/
    MY_FILE *my_file=malloc(sizeof(MY_FILE));
    my_file->data_loc = 0;
    my_file->FAT_LOC = fat_loc;
    my_file->DATA_SIZE=size;
    my_file->isEOF =false;
    my_file->pFAT_LOC=parent->FAT_LOC;
    //write_data
    write_data(my_file, data, size);
    my_file->data_loc = 0;
    parent->isEOF=false;
    parent->data_loc=0;
    return my_file;
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


uint16_t write_data(struct MY_FILE *p_file, void *data, uint16_t bytes){
    uint16_t bytes_wrote = 0;
    while(bytes_wrote<bytes) {
        //get location of data in block
        uint16_t block_loc = (uint16_t) (p_file->data_loc % BLOCK_SIZE);
        //set bytes to write to rest of block
        uint16_t bytes_to_copy = (uint16_t) (BLOCK_SIZE - block_loc);
        //set bytes what ever is smaller the remaining bytes or the rest of the block
        bytes_to_copy = bytes_to_copy <= (bytes-bytes_wrote) ? bytes_to_copy : (bytes-bytes_wrote);

        //copy disk to data
        memcpy(disk + get_disk_pos(p_file->FAT_LOC, p_file->data_loc),data+bytes_wrote,bytes_to_copy);
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

uint16_t get_free_block(uint16_t start) {
    int blocks = 1;
    while(blocks<TOTAL_BLOCKS) {
        blocks++;
        start = (uint16_t) ((start + 1) % TOTAL_BLOCKS);
        uint16_t test = fat_value(start);
        if(test==FREE_BLOCK){
            return start;
        }
    }
    exit(1);
}


void write_dir_entry(struct dir_entry entry,uint32_t location){
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

uint16_t fat_value(uint16_t block){
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