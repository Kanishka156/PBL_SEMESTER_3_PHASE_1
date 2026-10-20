#ifndef STACK_H
#define STACK_H

/* ------------------------------------------------------------
   stack.h - Integer stack (LIFO, array based)
   Used for: rebuilding a route in the right order (Dijkstra / BFS)
             and for iterative DFS
   ------------------------------------------------------------ */

#define MAX_STACK 256

typedef struct {
    int data[MAX_STACK];
    int top;              /* number of items in the stack */
} IntStack;

void stack_init(IntStack *s);
int  stack_is_empty(const IntStack *s);
int  stack_push(IntStack *s, int value);         /* 1 = ok, 0 = full  */
int  stack_pop(IntStack *s, int *out);           /* 1 = ok, 0 = empty */
int  stack_peek(const IntStack *s, int *out);    /* 1 = ok, 0 = empty */

#endif
