#ifndef HEAP_H
#define HEAP_H

/* ------------------------------------------------------------
   heap.h - Min-heap priority queue (array based)
   Used by Dijkstra: the node with the SMALLEST cost comes out first
   ------------------------------------------------------------ */

#define MAX_HEAP 512

typedef struct {
    int   node;       /* graph node id            */
    float priority;   /* cost so far (smaller = first) */
} HeapItem;

typedef struct {
    HeapItem items[MAX_HEAP];
    int      size;
} MinHeap;

void heap_init(MinHeap *h);
int  heap_is_empty(const MinHeap *h);
int  heap_push(MinHeap *h, int node, float priority);   /* 1 = ok, 0 = full */
int  heap_pop(MinHeap *h, HeapItem *out);                /* 1 = ok, 0 = empty */

#endif
