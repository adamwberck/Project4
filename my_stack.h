//
// Created by mandr on 2019-04-17.
//

#ifndef PROJECT4_MY_STACK_H
#define PROJECT4_MY_STACK_H
#include <stdint.h>

struct my_stack{
    uint16_t array[128];
    int size;
};

struct my_stack create_my_stack();

void put_my_stack(struct my_stack *stack, uint16_t element);

uint16_t pop_my_stack(struct my_stack *stack);
#endif //PROJECT4_MY_STACK_H
