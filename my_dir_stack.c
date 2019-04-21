//
// Created by mandr on 2019-04-17.
//

#include "my_dir_stack.h"

void put_my_dir_stack(struct my_dir_stack *stack,MY_FILE *element){
    stack->array[stack->size++] = element;
}

MY_FILE *pop_my_dir_stack(struct my_dir_stack *stack){
    return stack->array[--stack->size];
}

struct my_dir_stack create_my_dir_stack(){
    struct my_dir_stack stack;
    stack.size =0;
    return stack;
}