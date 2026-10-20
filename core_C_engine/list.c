#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

void history_init(History *h)
{
    h->head  = NULL;
    h->tail  = NULL;
    h->count = 0;
}

void history_free(History *h)
{
    HistoryNode *cur = h->head;
    while (cur != NULL) {
        HistoryNode *next = cur->next;
        free(cur);
        cur = next;
    }
    history_init(h);          /* also used as "Reset" */
}

int history_add(History *h, int from_node, int to_node, float cost,
                const char *note)
{
    HistoryNode *n = (HistoryNode *)malloc(sizeof(HistoryNode));
    if (n == NULL) return 0;

    n->id        = h->count + 1;
    n->from_node = from_node;
    n->to_node   = to_node;
    n->cost      = cost;
    strncpy(n->note, note, NOTE_LEN - 1);
    n->note[NOTE_LEN - 1] = '\0';
    n->next      = NULL;

    if (h->tail == NULL) {            /* list was empty */
        h->head = n;
    } else {
        h->tail->next = n;
    }
    h->tail = n;
    h->count++;
    return n->id;
}

const HistoryNode *history_get(const History *h, int index)
{
    if (index < 0 || index >= h->count) return NULL;

    const HistoryNode *cur = h->head;
    for (int i = 0; i < index; i++) cur = cur->next;
    return cur;
}

void history_print(const History *h, const Graph *g)
{
    if (h->count == 0) {
        printf("  (history is empty)\n");
        return;
    }
    for (const HistoryNode *n = h->head; n != NULL; n = n->next) {
        printf("  #%d  %s -> %s  cost %.2f  [%s]\n",
               n->id,
               g->nodes[n->from_node].name,
               g->nodes[n->to_node].name,
               n->cost, n->note);
    }
}
