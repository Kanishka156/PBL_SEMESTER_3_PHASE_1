#include "heap.h"

static void swap_items(HeapItem *a, HeapItem *b)
{
    HeapItem t = *a;
    *a = *b;
    *b = t;
}

void heap_init(MinHeap *h)
{
    h->size = 0;
}

int heap_is_empty(const MinHeap *h)
{
    return h->size == 0;
}

int heap_push(MinHeap *h, int node, float priority)
{
    if (h->size >= MAX_HEAP) return 0;

    /* put the new item at the end, then move it up */
    int i = h->size++;
    h->items[i].node = node;
    h->items[i].priority = priority;

    while (i > 0) {
        int parent = (i - 1) / 2;
        if (h->items[parent].priority <= h->items[i].priority) break;
        swap_items(&h->items[parent], &h->items[i]);
        i = parent;
    }
    return 1;
}

int heap_pop(MinHeap *h, HeapItem *out)
{
    if (h->size == 0) return 0;

    *out = h->items[0];

    /* move the last item to the root, then sift it down */
    h->items[0] = h->items[--h->size];

    int i = 0;
    while (1) {
        int left  = 2 * i + 1;
        int right = 2 * i + 2;
        int smallest = i;

        if (left < h->size &&
            h->items[left].priority < h->items[smallest].priority)
            smallest = left;
        if (right < h->size &&
            h->items[right].priority < h->items[smallest].priority)
            smallest = right;

        if (smallest == i) break;
        swap_items(&h->items[i], &h->items[smallest]);
        i = smallest;
    }
    return 1;
}
