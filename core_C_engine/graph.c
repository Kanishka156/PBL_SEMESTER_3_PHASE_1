#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graph.h"

static const float TRAFFIC_FACTOR[3] = { 1.0f, 1.5f, 2.5f };
static const char *TRAFFIC_NAME[3]   = { "clear", "moderate", "heavy" };

void graph_init(Graph *g)
{
    g->count = 0;
    for (int i = 0; i < MAX_NODES; i++) {
        g->nodes[i].edges = NULL;
    }
}

void graph_free(Graph *g)
{
    for (int i = 0; i < g->count; i++) {
        Edge *e = g->nodes[i].edges;
        while (e != NULL) {
            Edge *tmp = e;
            e = e->next;
            free(tmp);
        }
        g->nodes[i].edges = NULL;
    }
    g->count = 0;
}

int graph_add_node(Graph *g, const char *name, float x, float y, NodeType type)
{
    if (g->count >= MAX_NODES) return -1;

    Node *n = &g->nodes[g->count];
    n->id = g->count;
    strncpy(n->name, name, NAME_LEN - 1);
    n->name[NAME_LEN - 1] = '\0';
    n->x = x;
    n->y = y;
    n->type = type;
    n->edges = NULL;

    return g->count++;
}

/* inserts one directed edge at the head of u's list */
static int add_directed(Graph *g, int u, int v, float distance)
{
    Edge *e = (Edge *)malloc(sizeof(Edge));
    if (e == NULL) return 0;

    e->to       = v;
    e->distance = distance;
    e->traffic  = TRAFFIC_CLEAR;
    e->closed   = 0;
    e->next     = g->nodes[u].edges;
    g->nodes[u].edges = e;
    return 1;
}

int graph_add_road(Graph *g, int u, int v, float distance)
{
    if (u < 0 || v < 0 || u >= g->count || v >= g->count) return 0;
    if (u == v || distance <= 0.0f) return 0;
    if (graph_find_edge(g, u, v) != NULL) return 0;   /* already exists */

    if (!add_directed(g, u, v, distance)) return 0;
    if (!add_directed(g, v, u, distance)) return 0;
    return 1;
}

Edge *graph_find_edge(const Graph *g, int u, int v)
{
    if (u < 0 || u >= g->count) return NULL;

    for (Edge *e = g->nodes[u].edges; e != NULL; e = e->next) {
        if (e->to == v) return e;
    }
    return NULL;
}

int graph_set_traffic(Graph *g, int u, int v, TrafficLevel level)
{
    Edge *a = graph_find_edge(g, u, v);
    Edge *b = graph_find_edge(g, v, u);
    if (a == NULL || b == NULL) return 0;

    a->traffic = level;
    b->traffic = level;
    return 1;
}

int graph_set_closed(Graph *g, int u, int v, int closed)
{
    Edge *a = graph_find_edge(g, u, v);
    Edge *b = graph_find_edge(g, v, u);
    if (a == NULL || b == NULL) return 0;

    a->closed = closed ? 1 : 0;
    b->closed = closed ? 1 : 0;
    return 1;
}

float traffic_factor(TrafficLevel level)
{
    return TRAFFIC_FACTOR[level];
}

float edge_cost(const Edge *e, CostMode mode)
{
    if (e->closed) return INF_COST;
    if (mode == COST_DISTANCE) return e->distance;
    return e->distance * traffic_factor(e->traffic);
}

void graph_print(const Graph *g)
{
    printf("Graph: %d nodes\n", g->count);
    for (int i = 0; i < g->count; i++) {
        const Node *n = &g->nodes[i];
        printf("  [%2d] %-18s %s\n", n->id, n->name,
               n->type == NODE_HOSPITAL ? "(HOSPITAL)" : "");
        for (Edge *e = n->edges; e != NULL; e = e->next) {
            printf("        -> [%2d] %-18s %.1f km, %s%s\n",
                   e->to, g->nodes[e->to].name, e->distance,
                   TRAFFIC_NAME[e->traffic], e->closed ? ", CLOSED" : "");
        }
    }
}

void graph_load_sample_map(Graph *g)
{
    graph_init(g);

    /* id: name, x, y (screen coordinates for a ~1000x600 map area) */
    graph_add_node(g, "Central Chowk",    500, 300, NODE_JUNCTION);  /*  0 */
    graph_add_node(g, "Clock Tower",      350, 200, NODE_JUNCTION);  /*  1 */
    graph_add_node(g, "Railway Station",  150, 150, NODE_JUNCTION);  /*  2 */
    graph_add_node(g, "Bus Stand",        150, 350, NODE_JUNCTION);  /*  3 */
    graph_add_node(g, "Market",           350, 400, NODE_JUNCTION);  /*  4 */
    graph_add_node(g, "University",       650, 150, NODE_JUNCTION);  /*  5 */
    graph_add_node(g, "Park Gate",        650, 420, NODE_JUNCTION);  /*  6 */
    graph_add_node(g, "Old Bridge",       850, 300, NODE_JUNCTION);  /*  7 */
    graph_add_node(g, "Ring Road North",  500,  80, NODE_JUNCTION);  /*  8 */
    graph_add_node(g, "Ring Road South",  500, 520, NODE_JUNCTION);  /*  9 */
    graph_add_node(g, "City Hospital",    250, 520, NODE_HOSPITAL);  /* 10 */
    graph_add_node(g, "Metro Hospital",   850, 130, NODE_HOSPITAL);  /* 11 */
    graph_add_node(g, "General Hospital", 850, 500, NODE_HOSPITAL);  /* 12 */

    /* roads: u, v, distance in km */
    graph_add_road(g,  0,  1, 1.5f);
    graph_add_road(g,  0,  4, 1.8f);
    graph_add_road(g,  0,  5, 2.2f);
    graph_add_road(g,  0,  6, 2.0f);
    graph_add_road(g,  0,  7, 3.4f);
    graph_add_road(g,  1,  2, 2.6f);
    graph_add_road(g,  1,  3, 3.0f);
    graph_add_road(g,  1,  8, 2.3f);
    graph_add_road(g,  2,  3, 2.2f);
    graph_add_road(g,  3,  4, 2.4f);
    graph_add_road(g,  4,  9, 2.5f);
    graph_add_road(g,  4, 10, 2.8f);
    graph_add_road(g,  5,  7, 2.4f);
    graph_add_road(g,  5,  8, 2.6f);
    graph_add_road(g,  5, 11, 2.9f);
    graph_add_road(g,  6,  7, 2.5f);
    graph_add_road(g,  6,  9, 2.3f);
    graph_add_road(g,  6, 12, 2.7f);
    graph_add_road(g,  7, 11, 1.8f);
    graph_add_road(g,  7, 12, 2.0f);
    graph_add_road(g,  9, 10, 2.6f);
}
