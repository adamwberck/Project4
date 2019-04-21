//
// Created by Adam on 2019-04-17.
//

#ifndef PROJECT4_MY_DIR_STACK_H
#define PROJECT4_MY_DIR_STACK_H
#include <stdint.h>
#include <stdbool.h>


struct MY_FILE {
    uint16_t data_loc;
    uint16_t DATA_SIZE;
    uint16_t FAT_LOC;
    uint16_t pFAT_LOC;
    bool isEOF;
}typedef MY_FILE;

struct my_dir_stack{
    MY_FILE *array[1000];
    int size;
};

struct my_dir_stack create_my_dir_stack();

void put_my_dir_stack(struct my_dir_stack *stack, MY_FILE *element);

MY_FILE *pop_my_dir_stack(struct my_dir_stack *stack);
#endif //PROJECT4_MY_STACK_H
