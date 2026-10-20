#include <stdio.h>
#include "algorithms.h"
#include "heap.h"
#include "stack.h"
#include "queue.h"

/* ============================================================
   Helpers
   ============================================================ */

static PathResult empty_result(void)
{
    PathResult r;
    r.found  = 0;
    r.cost   = INF_COST;
    r.length = 0;
    return r;
}

/* Rebuilds the route from parent[] using a STACK.
   Walking back from dst gives the nodes in reverse order, so we push
   them on the stack and pop them to get src -> dst. */
static PathResult path_from_parent(const int parent[], int src, int dst,
                                   float cost)
{
    PathResult r = empty_result();
    IntStack st;
    stack_init(&st);

    for (int v = dst; v != -1; v = parent[v]) {
        if (!stack_push(&st, v)) return r;
    }

    int first;
    if (!stack_peek(&st, &first) || first != src) return r;

    int v;
    while (stack_pop(&st, &v)) {
        if (r.length >= MAX_NODES) return empty_result();
        r.path[r.length++] = v;
    }

    r.cost  = cost;
    r.found = 1;
    return r;
}

/* ============================================================
   Dijkstra
   ============================================================ */

void dijkstra_run(const Graph *g, int src, CostMode mode,
                  float dist[], int parent[])
{
    for (int i = 0; i < MAX_NODES; i++) {
        dist[i]   = INF_COST;
        parent[i] = -1;
    }
    if (src < 0 || src >= g->count) return;

    MinHeap heap;
    heap_init(&heap);

    dist[src] = 0.0f;
    heap_push(&heap, src, 0.0f);

    while (!heap_is_empty(&heap)) {
        HeapItem top;
        heap_pop(&heap, &top);

        int   u = top.node;
        float d = top.priority;

        /* stale entry: a cheaper path to u was already found */
        if (d > dist[u]) continue;

        for (Edge *e = g->nodes[u].edges; e != NULL; e = e->next) {
            if (e->closed) continue;                 /* blocked road */

            float nd = d + edge_cost(e, mode);
            if (nd < dist[e->to]) {
                dist[e->to]   = nd;
                parent[e->to] = u;
                heap_push(&heap, e->to, nd);
            }
        }
    }
}

PathResult find_path(const Graph *g, int src, int dst, CostMode mode)
{
    float dist[MAX_NODES];
    int   parent[MAX_NODES];

    if (dst < 0 || dst >= g->count) return empty_result();

    dijkstra_run(g, src, mode, dist, parent);
    if (dist[dst] >= INF_COST) return empty_result();

    return path_from_parent(parent, src, dst, dist[dst]);
}

PathResult find_nearest_hospital(const Graph *g, int src, CostMode mode)
{
    float dist[MAX_NODES];
    int   parent[MAX_NODES];

    dijkstra_run(g, src, mode, dist, parent);

    int best = -1;
    for (int i = 0; i < g->count; i++) {
        if (g->nodes[i].type != NODE_HOSPITAL) continue;
        if (dist[i] >= INF_COST) continue;
        if (best == -1 || dist[i] < dist[best]) best = i;
    }

    if (best == -1) return empty_result();
    return path_from_parent(parent, src, best, dist[best]);
}

/* ============================================================
   BFS
   ============================================================ */

/* Core BFS. hops[i] = number of roads from src (-1 = not reached). */
static void bfs_run(const Graph *g, int src, int parent[], int hops[],
                    int order[], int *count)
{
    for (int i = 0; i < MAX_NODES; i++) {
        parent[i] = -1;
        hops[i]   = -1;
    }
    *count = 0;
    if (src < 0 || src >= g->count) return;

    IntQueue q;
    queue_init(&q);

    hops[src] = 0;
    enqueue(&q, src);

    int u;
    while (dequeue(&q, &u)) {
        order[(*count)++] = u;

        for (Edge *e = g->nodes[u].edges; e != NULL; e = e->next) {
            if (e->closed) continue;
            if (hops[e->to] == -1) {          /* not visited yet */
                hops[e->to]   = hops[u] + 1;
                parent[e->to] = u;
                enqueue(&q, e->to);
            }
        }
    }
}

void bfs_order(const Graph *g, int src, int order[], int *count)
{
    int parent[MAX_NODES], hops[MAX_NODES];
    bfs_run(g, src, parent, hops, order, count);
}

PathResult find_fewest_roads_path(const Graph *g, int src, int dst)
{
    int parent[MAX_NODES], hops[MAX_NODES], order[MAX_NODES], count;

    if (dst < 0 || dst >= g->count) return empty_result();

    bfs_run(g, src, parent, hops, order, &count);
    if (hops[dst] == -1) return empty_result();

    return path_from_parent(parent, src, dst, (float)hops[dst]);
}

int is_reachable(const Graph *g, int src, int dst)
{
    int parent[MAX_NODES], hops[MAX_NODES], order[MAX_NODES], count;

    if (dst < 0 || dst >= g->count) return 0;

    bfs_run(g, src, parent, hops, order, &count);
    return hops[dst] != -1;
}

/* ============================================================
   DFS
   ============================================================ */

/* Iterative DFS using our own stack */
void dfs_order(const Graph *g, int src, int order[], int *count)
{
    int visited[MAX_NODES] = {0};
    *count = 0;
    if (src < 0 || src >= g->count) return;

    IntStack st;
    stack_init(&st);
    stack_push(&st, src);

    int u;
    while (stack_pop(&st, &u)) {
        if (visited[u]) continue;
        visited[u] = 1;
        order[(*count)++] = u;

        for (Edge *e = g->nodes[u].edges; e != NULL; e = e->next) {
            if (!e->closed && !visited[e->to]) {
                stack_push(&st, e->to);
            }
        }
    }
}

/* Recursive DFS with backtracking: counts every simple route u -> dst.
   (Fine for a small city map; the count grows quickly on big graphs.) */
static int count_routes_rec(const Graph *g, int u, int dst, int visited[])
{
    if (u == dst) return 1;

    visited[u] = 1;
    int total = 0;
    for (Edge *e = g->nodes[u].edges; e != NULL; e = e->next) {
        if (!e->closed && !visited[e->to]) {
            total += count_routes_rec(g, e->to, dst, visited);
        }
    }
    visited[u] = 0;                    /* backtrack */
    return total;
}

int dfs_count_routes(const Graph *g, int src, int dst)
{
    if (src < 0 || src >= g->count || dst < 0 || dst >= g->count) return 0;

    int visited[MAX_NODES] = {0};
    return count_routes_rec(g, src, dst, visited);
}

/* ============================================================
   Alternate route
   ============================================================ */

PathResult find_alternate_path(Graph *g, int src, int dst, CostMode mode,
                               const PathResult *primary)
{
    PathResult best = empty_result();
    if (!primary->found || primary->length < 2) return best;

    for (int i = 0; i < primary->length - 1; i++) {
        int u = primary->path[i];
        int v = primary->path[i + 1];

        Edge *e = graph_find_edge(g, u, v);
        if (e == NULL) continue;

        int was_closed = e->closed;
        graph_set_closed(g, u, v, 1);                 /* block this road */
        PathResult r = find_path(g, src, dst, mode);
        graph_set_closed(g, u, v, was_closed);        /* restore it      */

        if (r.found && (!best.found || r.cost < best.cost)) best = r;
    }
    return best;
}

/* ============================================================
   Printing
   ============================================================ */

void path_print(const Graph *g, const PathResult *r)
{
    if (!r->found) {
        printf("    No route available.\n");
        return;
    }
    printf("    ");
    for (int i = 0; i < r->length; i++) {
        printf("%s%s", g->nodes[r->path[i]].name,
               i < r->length - 1 ? " -> " : "");
    }
    printf("\n    cost = %.2f\n", r->cost);
}
