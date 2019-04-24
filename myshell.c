#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <ctype.h>
#include <stdbool.h>
#include <wait.h>
#include <dirent.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include "myshell.h"
#include "disk.h"
#include "user_commands.h"

#define FD_READ_END 0
#define FD_WRITE_END 1


char** paths;
int number_of_paths = 0;
int fd[2];

MY_FILE *opened_file;
char *current_disk;
int main() {
    int repeat =0;
    current_disk = "NO DISK";
    current_dir_name="";
    do{
        char *prompt = get_prompt();
        char *input;
        get_input(prompt, &input);
        int count = get_count(input);
        //parsed_input list of char* pointers
        char **parsed_input = parse_input(input, count);
        my_built_in(count,parsed_input);
        free(parsed_input);
        repeat++;
    }while(repeat<100000000);//keep going unless end of file, always go if interactive mode
    return(0);
}

char *get_prompt() {
    char *prompt = malloc(sizeof(char)*(10+1+strlen(current_dir_name)+strlen(current_disk)));
    strcpy(prompt,current_disk);
    strcat(prompt,"|");
    strcat(prompt,current_dir_name);
    strcat(prompt,"~FAT shell>");
    return prompt;
}


//use last argument as write redirect argument
void perform_write_redirect(char *const *args, int count, int write_redirect) {
    int f=0;
    //truncate
    if(write_redirect==1) {
        f = open(args[count - 1], O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    //append
    }else{
        f = open(args[count - 1], O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
    }
    dup2(f,STDOUT_FILENO);//stdout goto file;
    dup2(f,STDERR_FILENO);//std error goto
    close(f);
}

//function does built in commands
bool my_built_in(int count, char** args) {
    char *cmd = args[0];
    //exit or quit
    if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
        exit(0);
    //cd
    } else if (strcmp(cmd, "cd") == 0) {
        change_dir(args[1]);
    //clear screen
    } else if (strcmp(cmd, "clr") == 0) {
        //goto position(1,1) then clear the screen
        printf("\e[1;1H\e[2J");
        //
    } else if (strcmp(cmd, "mount") == 0) {
        struct boot *my_boot = malloc(sizeof(struct boot));
        strcpy(my_boot->valid_check, "This is Adam's FAT Drive");
        //using mmap to open my_disk
        int int_disk = open(args[1], O_RDWR, 0);
        disk = mmap(NULL, TOTAL_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, int_disk, 0);
        if (memcmp(my_boot->valid_check, disk, VALID_CHECK_SIZE) != 0) {
            printf("Not correctly formatted disk\n");
            return true;
        }
        current_disk=args[1];
        //set current dir to root folder
        mount();
    } else if (strcmp(cmd, "open") == 0) {
        char **file_name_ext = parse_file_and_extension(args[1]);
        opened_file = user_open_file(current_dir, file_name_ext[0], file_name_ext[1]);
        if (opened_file != NULL) {
            printf("File %s.%s opened\n",file_name_ext[0],file_name_ext[1]);
        }else{
            printf("open failed\n");
        }
    } else if (strcmp(cmd, "close") == 0) {
        close_file(opened_file);
        opened_file=NULL;
        printf("File closed\n");
    } else if (strcmp(cmd,"display")==0){
        display_everything();
    } else if (strcmp(cmd,"create")==0){
        //open file and write it two interior disk
        FILE *test_file = fopen(args[2],"r");
        int test_file_size = (int) fsize(args[2]);
        printf("size %d\n",  test_file_size);
        char *my_test_file_data = malloc(sizeof(char)*test_file_size);
        fread(my_test_file_data, sizeof(char), (size_t) test_file_size, test_file);
        fclose(test_file);

        char **file_name_ext = parse_file_and_extension(args[1]);
        opened_file = user_create_file(current_dir, file_name_ext[0], file_name_ext[1], my_test_file_data,
                                       (uint16_t) test_file_size);
        if (opened_file != NULL) {
            printf("File %s.%s created\n",file_name_ext[0],file_name_ext[1]);
        }else{
            printf("create failed\n");
        }

        free(my_test_file_data);
    } else if(strcmp(cmd,"read")==0){
        if(opened_file!=NULL) {
            display_file_data(opened_file);
        }else{
            printf("No file opened\n");
        }
    } else if(strcmp(cmd,"delete")==0){
        char **file_name_ext = parse_file_and_extension(args[1]);
        user_delete_file(current_dir, file_name_ext[0], file_name_ext[1]);
    } else if(strcmp(cmd,"copy")==0){
        char **file_name_ext = parse_file_and_extension(args[1]);
        user_copy_file(current_dir,opened_file,file_name_ext[0],file_name_ext[1]);
    } else if(strcmp(cmd,"mkdir")==0){
        make_dir(current_dir,args[1]);
    } else if(strcmp(cmd,"rmdir")==0){
        delete_dir(current_dir,args[1]);
    } else if(strcmp(cmd,"disk")==0){
        disk_main(args[1]);
    }
}
//counts the different arguments in input including command
int get_count(char *input) {
    bool part_of_word = false;
    int count=0;
    for(int i=0;i<strlen(input);i++){
        if(!isspace(input[i])&&!part_of_word){
            part_of_word=true;
            count++;
        }
        else if(isspace(input[i])){
            part_of_word=false;
        }
    }
    return count;
}

//parses input into char** (a char array);
char** parse_input(char* input,int count){
    char** output    = 0;
    char del[2]=" ";

    output = malloc(sizeof(char*) * (count+2));
    int j  = 0;
    char* token = strtok(input, del);
    while (token)
    {
        *(output + j++) = strdup(token);
        token = strtok(0, del);
    }
    *(output+ j) = 0;

    return output;
}

char** parse_file_and_extension(char *input){
    char** output    = 0;
    char del[2]=".";

    output = malloc(sizeof(char*) * (4));
    int j  = 0;
    char* token = strtok(input, del);
    while (token)
    {
        *(output + j++) = strdup(token);
        token = strtok(0, del);
    }
    *(output+ j) = 0;

    return output;
}

//gets input from user
void get_input(char* prompt,char **input){
    size_t buffer_size = 32;//aprox size of a command; can be changed by getline if not large enough
    *input = (char *) malloc(buffer_size * sizeof(char)); //allocate space for input
    printf("%s",prompt);
    getline(input, &buffer_size, stdin);//getline
    remove_newline_char(input);
    replace_other_whitespace(input);
}

