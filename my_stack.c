//
// Created by mandr on 2019-04-17.
//

#include "my_stack.h"

void put_my_stack(struct my_stack *stack,uint16_t element){
    stack->array[stack->size++] = element;
}

uint16_t pop_my_stack(struct my_stack *stack){
    return stack->array[--stack->size];
}

struct my_stack create_my_stack(){
    struct my_stack stack;
    stack.size =0;
    return stack;
}