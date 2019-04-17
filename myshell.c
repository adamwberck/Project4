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
#include "myshell.h"
#include "helper.h"

#define FD_READ_END 0
#define FD_WRITE_END 1

extern char** environ;

char** paths;
int number_of_paths = 0;
int fd[2];

int shell_main(int argc, char* argv[]) {
    put_info_into_env();

    FILE *file = 0;
    char line[256];
    if (argc > 1) {
        char const *const fileName = argv[1];
        file = fopen(fileName, "r");
        fgets(line, sizeof(line), file);
    }
    do{
        char *prompt = get_prompt();
        char *input;
        if(argc>1){
            input = malloc(sizeof(char)*strlen(line));
            sprintf(input,"%s", line);
            remove_newline_char(&input);
        }else {
            get_input(prompt, &input);
        }
        int count = get_count(input);
        //parsed_input list of char* pointers
        char **parsed_input = parse_input(input, count);
        process_input(count, parsed_input);
        free(parsed_input);
    }while(argc<=1 || fgets(line, sizeof(line), file));//keep going unless end of file, always go if interactive mode
    fclose(file);
    return(0);
}

//process input and run commands
void process_input(int count, char *const *parsed_input) {
    char extra = '\0';//extra keeps track of special ways of executing commands
    char **single_command = malloc(sizeof(char*)*(count+2));//one command that can be part of execv
    int cmd_cnt = 0;//number of separate commands
    int args_cnt=0;//command count keeps track of arguments in one single command
    bool piping_in = false;//keeps track of piping in read redirect;
    for(int i=0;i<count;i++){
        //make wrd set to first word of input
        char *wrd = parsed_input[i];
        if(strcmp(wrd,"&")==0){
            extra = '&';
        }else if(strcmp(wrd,"|")==0){
            extra = '|';
        }
        if (extra=='\0'){
            //add word to command
            *(single_command + args_cnt++) = strdup(wrd);
        }
        //checks if &/| separator or end of arguments
        if(extra != '\0' || ((i+1) >= count)){
            //execute the single command
            execute(single_command,args_cnt,extra,piping_in);
            piping_in = extra=='|';//if piping out then next time its piping in.
            cmd_cnt++;
            //zero out single command; not doing this cause issues when performing second execv;
            for(int m=0;m<args_cnt;m++){
                single_command[m]=NULL;
            }
            if((i+1) >= count &&  extra!='&'){// if end of the line and not background exe
                cmd_cnt--;//last command already waited in execute;
                while(cmd_cnt>0) {
                    wait(NULL);//wait until exe f
                    cmd_cnt--;
                }
            }
            extra='\0';//reset extra
            args_cnt=0;//set command count to 0;
        }
    }
    //close(fd[FD_READ_END]);
    //close(fd[FD_WRITE_END]);
    free(single_command);
}

char *get_prompt() {
    char dir[PATH_MAX];
    if (getcwd(dir, sizeof(dir)) == NULL) {
        printf("getcwd failed\n");//never should run
    }
    //get position of / to find relative dir.
    size_t i=strlen(dir)-1;
    while(dir[i]!='/'&&i>=0){
        i--;
    }
    char *prompt = malloc(PATH_MAX* sizeof(char));
    strcpy(prompt,"\n");
    char *mid_prompt = malloc(strlen(dir)* sizeof(char));
    strcpy(mid_prompt,dir+i);
    char * end_prompt = " ~:myshell> ";
    strcat(prompt,mid_prompt);
    strcat(prompt,end_prompt);
    return prompt;
}

void put_info_into_env() {
    //allocate space to hold shell environment
    size_t buffer_size=PATH_MAX;
    char* temp = malloc(buffer_size* sizeof(char));
    readlink("/proc/self/exe",temp,buffer_size);
    //remove exe name
    remove_exe_name(&temp);
    //add '/myshell' to the end of name
    strcat(temp,"/myshell");
    //allocate space for shell
    char* shell=malloc((strlen(temp)+7)* sizeof(char));
    //prefix concat 'shell=' to environment
    strcpy(shell,"shell=");
    strcat(shell,temp);
    //put shell into environment
    if(putenv(shell)!=0){
        //putenv failed
        my_error();
    }

    char* parent=malloc((strlen(temp)+8)* sizeof(char));
    //prefix concat 'shell=' to environment
    strcpy(parent,"parent=");
    strcat(parent,temp);
    //put parent into environment
    if(putenv(parent)!=0){
        //putenv failed
        my_error();
    }


    free(temp);//free temp memory;
}


//args is arguments, count is number of arguments and extra is if to pipe or to run in parallel
void execute(char **args ,int count ,char extra ,bool piping_in ){
    //checks if built in command
    if(my_built_in(count,args)){
        return;
    }
    char *tmp =  strdup(args[0]);
    bool working_path = false;
    int i=0;
    //go through paths until command is found
    while(!working_path && i<number_of_paths){
        char *p = strdup(paths[i++]);
        strcat(p,tmp);
        //test if it exists and i have execute permission.
        if(access(p,X_OK)==0){
            working_path=true;
        }
    }
    if(!working_path) {
        printf("Command %s not found.\n",args[0]);
    } else {
        char *path = paths[i - 1];
        //put path on the begging of buffer
        strcpy(args[0], path);
        strcat(args[0], tmp);
        free(tmp);//free tmp
        //check if has a write redirect
        int write_redirect = has_write_redirect(args, count);
        if (write_redirect == -1) {
            printf("Error: malformed write_redirect\n");
            return;
        }
        //check if has a read redirect
        int read_redirect = has_read_redirect(args, count);
        //put in null terminator
        args[count] = NULL;
        if (extra == '|') {
            pipe(fd);
        }
        pid_t pid = fork();
        if (pid == 0) {
            //this is child
            if(extra == '|'){//pipe out
                dup2(fd[FD_WRITE_END],STDOUT_FILENO);
                close(fd[FD_READ_END]);
                close(fd[FD_WRITE_END]);
            }
            if (write_redirect>=1){
                perform_write_redirect(args, count, write_redirect);
                //remove write redirect from arguments
                args[count-1] = 0;
                args[count-2] = 0;
                count-=2;
            }
            if(piping_in){
                dup2(fd[FD_READ_END],STDIN_FILENO);
                close(fd[FD_READ_END]);
                close(fd[FD_WRITE_END]);
            }
            if (read_redirect==1){
                //read only open of input file
                int f = open(args[count - 1], O_RDONLY);
                dup2(f,STDIN_FILENO);//stdout goto file;
                close(f);
                //remove read redirect from arguments
                args[count-1] = 0;
                args[count-2] = 0;
            }
            //run code
            execv(args[0], args);
            printf("exe failed %d \n",errno);
            //my_error();//only runs if execv fails; never should run
        } else if (pid < 0) {
            printf("fork failed");
        } else {
            //this is the parent
            if(extra!='&' && extra!='|') {//don't wait if piping or parallel
                if(piping_in){
                    close(fd[FD_WRITE_END]);
                    close(fd[FD_READ_END]);
                }
                wait(NULL);//wait until child is done
            }
        }
    }
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

//check if it has a read redirect 1 if it does 0 it does not -1 if error
int has_read_redirect(char **args, int count) {
    //check if has write redirect to change argument check
    if(has_write_redirect(args,count)>=1){
        count-=2;
    }
    int is_redirect=0;
    for(int i=0;i<count;i++){
        char *temp = args[i];
        if(temp[0]=='<'&&strlen(temp)==1){
            //checks if redirect is malformed
            if(is_redirect == 0 && count-2==i) {
                is_redirect = 1;
            }else{
                return -1;
            }
        }
    }
    return is_redirect;
}

//returns 1 if redirect replace,2 if redirect append 0 if not, and -1 if error;
int has_write_redirect(char **args, int count) {
    int is_redirect=0;
    for(int i=0;i<count;i++){
        char *temp = args[i];
        if(temp[0]=='>'&&strlen(temp)<=2){
            //checks if redirect is malformed
            if(is_redirect == 0 && count-2==i) {
                //check if append or replace
                if(temp[1]=='>' && strlen(temp)==2){
                    is_redirect = 2;//append
                }else {
                    is_redirect = 1;//replace
                }
            }else{
                return -1;
            }
        }
    }
    return is_redirect;
}

// my_cd uses chdir() to change director
void my_cd(int count,char** args){
    //if count more than two error
    if (count>2){
        //cd takes one argument
        my_error();
    }
        //other wise run cd
    else{
        char *new_dir;
        //one argument give own directory by using period;
        if(count==1){
            new_dir=".";
            //otherwise give first argument
        }else{
            new_dir=args[1];
        }
        if(chdir(new_dir)==-1) {
            //cd fails
            my_error();
        }else{
            char cwd[PATH_MAX];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                char* PWD = malloc((strlen(cwd)+7)* sizeof(char));
                //prefix concat 'shell=' to environment
                strcpy(PWD,"PWD=");
                strcat(PWD,cwd);
                putenv(PWD);
            } else {
                perror("getcwd() error");
            }
        }
    }
}

//my dir using readdir to display content of directory
void my_dir(int count, char ** args) {
    if(count == 2) {
        DIR *d;
        struct dirent *dir;
        d = opendir(args[1]);
        if (d) {
            while ((dir = readdir(d)) != NULL) {
                write(STDOUT_FILENO,dir->d_name,strlen(dir->d_name));
                write(STDOUT_FILENO,"\n",1);
            }
            closedir(d);
        }
    }else{
        //Correct usage: dir <directory>
        my_error();
    }
}

//function does built in commands
bool my_built_in(int count, char** args) {
    char *cmd = args[0];
    //exit or quit
    if(strcmp(cmd,"exit")==0 || strcmp(cmd,"quit")==0){
        exit(0);
    //cd
    }else if(strcmp(cmd,"cd")==0) {
        my_cd(count,args);
    //path
    }else if(strcmp(cmd,"path")==0) {
        //handle memory allocation for paths
        free(paths);
        paths = malloc((size_t) (count) * sizeof(char));
        //write each argument as a path
        for (int i = 1; i < count; i++) {
            paths[i - 1] = malloc(strlen(args[i]) * sizeof(char));
            paths[i - 1] = args[i];//set paths
            number_of_paths = count - 1;
        }
    //clears screen
    } else if(strcmp(cmd,"clr")==0) {
        //goto position(1,1) then clear the screen
        printf("\e[1;1H\e[2J");

    //possible output redirection
    }else if(strcmp(cmd,"dir")==0||strcmp(cmd,"environ")==0||(strcmp(cmd,"echo")==0)||strcmp(cmd,"help")==0){
        int write_redirect = has_write_redirect(args,count);
        int old_out = 0;
        if(write_redirect>=1){
            old_out = dup(STDOUT_FILENO);
            perform_write_redirect(args,count,write_redirect);
            count-=2;
        }
        //list contents of directory
        if(strcmp(cmd,"dir")==0){
            my_dir(count, args);
        //list environment vars
        } else if(strcmp(cmd,"environ")==0){
            int i=0;
            while(environ[i]){
                write(STDOUT_FILENO,environ[i],strlen(environ[i]));
                write(STDOUT_FILENO,"\n",1);
                i++;
            }
        //echo: repeat entered commands
        } else if(strcmp(cmd,"echo")==0){
            for(int i=1;i<count;i++) {
                write(STDOUT_FILENO,args[i],strlen(args[i]));
                write(STDOUT_FILENO," ",1);
            }
            write(STDOUT_FILENO,"\n",1);
        } else if(strcmp(cmd,"help")==0) {
            FILE* h_file = fopen("readme","r");
            char line[256];
            while (fgets(line, sizeof(line), h_file)) {
                write(STDOUT_FILENO,line,strlen(line));
            }
        }
        //reset STDOUT
        if(write_redirect>=1){
            dup2(old_out,STDOUT_FILENO);
        }
    //pause
    } else if(strcmp(cmd,"pause")==0) {
        printf("Paused. Press Enter to Continue.");
        //stays in while until enter
        while (getchar() != '\n'&& getchar() != '\r');
    } else{
        //not built in
        return false;
    }
    //was built in don't execute a command
    return true;
}

//displays the only allowed error
void my_error() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO,error_message,strlen(error_message));
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

//gets input from user
void get_input(char* prompt,char **input){
    size_t buffer_size = 32;//aprox size of a command; can be changed by getline if not large enough
    *input = (char *) malloc(buffer_size * sizeof(char)); //allocate space for input
    printf("%s",prompt);
    getline(input, &buffer_size, stdin);//getline
    remove_newline_char(input);
    replace_other_whitespace(input);
}

