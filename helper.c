//
// Created by Adam Berck on 2019-03-28.
//

#include "helper.h"

//finds newline char and replaces it with null terminator
void remove_newline_char(char **str){
    char *s = *str;//get pointer to char
    int i=0;
    //loop through string
    while(s[i]!='\0'){//stop looping when you get to null terminator
        //replace newline with null terminator
        if(s[i]=='\n'||s[i]=='\r'){
            s[i]='\0';
        }
        i++;
    }
}
//check if char* array is in order
bool array_in_order(char** array) {
    int i=0;
    while(array[i + 1] != NULL){
        if(strcmp(array[i],array[i+1])>0){
            return false;
        }
        i++;
    }
    return true;
}
//count file lines for allocating space
int count_file_lines(FILE *file) {
    char line[256];
    int i=0;
    while(fgets(line, sizeof(line), file)){//keep going unless end of file
        i++;
    }
    return i;
}
//print array
void print_array(char **array){
    int i=0;
    while(array[i]!=NULL)
        printf("%s\n",array[i++]);
}

//find in array using linear or binary search depending of if in order or not
bool find_in_array(char **array,char *element,bool in_order,int size){
    if(in_order){
        return find_array_b(array,element,0,size);
    }else{
        return find_array_l(array,element);
    }
}

//find in array using binary search, recursive method
bool find_array_b(char **array,char *element,int i,int j){
    if (j >= i) {
        //mid index is left bound plus half the difference
        int mid = i + (j - i) / 2;
        // check if element is in middle
        if (strcmp(array[mid],element)==0)
            return true;
        // is it in the left
        if (strcmp(array[mid],element)>0) {
            return find_array_b(array, element, i, mid - 1);
        // then its in the right
        }else {
            return find_array_b(array, element, mid + 1, j);
        }
    }
    //not in array
    return false;
}

//linear search an unsorted NULL terminated word list
bool find_array_l(char **array,char *element){
    int i=0;
    while(array[i]!=NULL) {
        if (strcmp(array[i], element) == 0) {
            return true;
        }
        i++;
    }
    return false;
}

//read file as and array
void read_file_as_array(char ***p_array, FILE *file) {
    char** array = *p_array;
    char line[256];
    int i=0;
    while(fgets(line, sizeof(line), file)){//keep going unless end of file
        char *input;
        input = malloc(sizeof(char) * (strlen(line)+1));
        sprintf(input,"%s", line);
        remove_newline_char(&input);
        array[i++] = strdup(input);
    }
    array[i]= NULL;
}

//set up the the ability to connect
int open_listen_fd(int port) {
    int listenfd, optval=1;
    struct sockaddr_in server_address;
    //set listenfd to socket
    if((listenfd= socket(AF_INET, SOCK_STREAM, 0)) < 0){
        return -1;
    }
    //set sockopt;
    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int)) < 0){
        return -1;
    }

    //erase the data at server address
    bzero((char *) &server_address, sizeof(server_address));
    //set up server address
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons((unsigned short)port);
    //bind server
    if((bind(listenfd,(struct sockaddr*)&server_address,sizeof(server_address)) < 0)){
        return -1;
    }
    //check listen
    if(listen(listenfd, 20) < 0){
        return -1;
    }
    return listenfd;
}

//remove exe name from path by looking at last '/'
int remove_exe_name(char **str) {
    char* s = *str;
    size_t len =  strlen(s);
    for(size_t i=len-1;i>=0;i--) {
        if(s[i] == '/'){
            s[i]='\0';
            return 0;
        }
    }
    return -1;
}


//replace tabs white space with a space
void replace_other_whitespace(char **str) {
    char *s = *str;//get pointer to char
    int i=0;
    //loop through string
    while(s[i]!='\0'){//stop looping when you get to null terminator
        //replace h-tab v-tab and form feed with space
        if(s[i]=='\t'||s[i]=='\v'||s[i]=='\f'){
            s[i]=' ';
        }
        i++;
    }
}