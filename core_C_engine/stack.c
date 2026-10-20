#include "stack.h"

void stack_init(IntStack *s)
{
    s->top = 0;
}

int stack_is_empty(const IntStack *s)
{
    return s->top == 0;
}

int stack_push(IntStack *s, int value)
{
    if (s->top >= MAX_STACK) return 0;
    s->data[s->top++] = value;
    return 1;
}

int stack_pop(IntStack *s, int *out)
{
    if (s->top == 0) return 0;
    *out = s->data[--s->top];
    return 1;
}

int stack_peek(const IntStack *s, int *out)
{
    if (s->top == 0) return 0;
    *out = s->data[s->top - 1];
    return 1;
}
