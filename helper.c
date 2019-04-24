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