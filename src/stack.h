#ifndef __STACK_H__
#define __STACK_H__

#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

typedef struct StackElement
{
    void* data;
    struct StackElement* next;
} StackElement;

typedef struct 
{
    StackElement* tail;
} Stack;

Stack* new_stack();
void push(Stack* stack, void* data);
void* pop(Stack* stack);
void* peek(Stack* stack);

#endif
