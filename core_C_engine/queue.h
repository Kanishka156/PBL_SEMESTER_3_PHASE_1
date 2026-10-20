#ifndef QUEUE_H
#define QUEUE_H

/* ------------------------------------------------------------
   queue.h - Integer queue (FIFO, circular array)
   Used for: BFS, and later for emergencies waiting for an ambulance
   ------------------------------------------------------------ */

#define MAX_QUEUE 64

typedef struct {
    int data[MAX_QUEUE];
    int head;             /* index of the front item      */
    int tail;             /* index where the next item goes */
    int count;            /* items currently in the queue */
} IntQueue;

void queue_init(IntQueue *q);
int  queue_is_empty(const IntQueue *q);
int  queue_is_full(const IntQueue *q);
int  enqueue(IntQueue *q, int value);     /* 1 = ok, 0 = full  */
int  dequeue(IntQueue *q, int *out);      /* 1 = ok, 0 = empty */

#endif
