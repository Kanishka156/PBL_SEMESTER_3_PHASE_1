#ifndef LIST_H
#define LIST_H

#include "graph.h"

/* ------------------------------------------------------------
   list.h - Singly linked list: dispatch HISTORY
   Every completed or re-routed dispatch is appended at the tail.
   ------------------------------------------------------------ */

#define NOTE_LEN 40

typedef struct HistoryNode {
    int    id;                 /* record number (1, 2, 3 ...)       */
    int    from_node;          /* where the emergency was           */
    int    to_node;            /* hospital the ambulance went to    */
    float  cost;               /* route cost                        */
    char   note[NOTE_LEN];     /* e.g. "critical", "rerouted"       */
    struct HistoryNode *next;
} HistoryNode;

typedef struct {
    HistoryNode *head;
    HistoryNode *tail;
    int          count;
} History;

void history_init(History *h);
void history_free(History *h);

/* appends a record at the end; returns the new record id, or 0 on failure */
int  history_add(History *h, int from_node, int to_node, float cost,
                 const char *note);

/* returns the record at position index (0 = oldest), or NULL */
const HistoryNode *history_get(const History *h, int index);

void history_print(const History *h, const Graph *g);

#endif
