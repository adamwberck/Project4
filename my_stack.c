//
// Created by mandr on 2019-04-17.
//

#include "my_stack.h"

void put_my_stack(struct my_stack *stack,unsigned short int element){
    stack->array[stack->size++] = element;
}

unsigned short int pop_my_stack(struct my_stack *stack){
    return stack->array[--stack->size];
}

struct my_stack create_my_stack(){
    struct my_stack stack;
    stack.size =0;
    return stack;
}