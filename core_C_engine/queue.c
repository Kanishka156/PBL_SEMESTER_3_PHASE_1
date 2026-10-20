#include "queue.h"

void queue_init(IntQueue *q)
{
    q->head  = 0;
    q->tail  = 0;
    q->count = 0;
}

int queue_is_empty(const IntQueue *q)
{
    return q->count == 0;
}

int queue_is_full(const IntQueue *q)
{
    return q->count == MAX_QUEUE;
}

int enqueue(IntQueue *q, int value)
{
    if (queue_is_full(q)) return 0;

    q->data[q->tail] = value;
    q->tail = (q->tail + 1) % MAX_QUEUE;      /* wrap around */
    q->count++;
    return 1;
}

int dequeue(IntQueue *q, int *out)
{
    if (queue_is_empty(q)) return 0;

    *out = q->data[q->head];
    q->head = (q->head + 1) % MAX_QUEUE;      /* wrap around */
    q->count--;
    return 1;
}
