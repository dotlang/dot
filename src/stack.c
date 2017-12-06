#include "stack.h"


Stack* new_stack()
{
    ALLOC(stack, Stack);
    return stack;
}

void push(Stack* stack, void* data)
{
    ALLOC(element, StackElement);
    element->data = data;
    element->next = stack->tail;
    stack->tail = element;
}

void* pop(Stack* stack)
{
    if ( stack->tail == NULL ) return NULL;
    StackElement* tail = stack->tail;

    stack->tail = stack->tail->next;
    return tail->data;
}

void* peek(Stack* stack)
{
    if ( stack->tail == NULL ) return NULL;
    return stack->tail->data;
}

