#ifndef GRAPH_H
#define GRAPH_H

/* ------------------------------------------------------------
   graph.h - City road network (weighted graph, adjacency list)
   Nodes  = intersections / hospitals
   Edges  = roads (each road is stored in BOTH directions)
   ------------------------------------------------------------ */

#define MAX_NODES 20
#define NAME_LEN  24
#define INF_COST  1e9f

typedef enum { NODE_JUNCTION = 0, NODE_HOSPITAL } NodeType;

typedef enum { TRAFFIC_CLEAR = 0, TRAFFIC_MODERATE, TRAFFIC_HEAVY } TrafficLevel;

/* COST_DISTANCE -> shortest route (distance only)
   COST_TRAFFIC  -> fastest route (distance x traffic factor)      */
typedef enum { COST_DISTANCE = 0, COST_TRAFFIC } CostMode;

typedef struct Edge {
    int           to;        /* destination node id               */
    float         distance;  /* road length (km)                  */
    TrafficLevel  traffic;   /* current traffic level             */
    int           closed;    /* 1 = blocked / accident / works    */
    struct Edge  *next;      /* next edge in the linked list      */
} Edge;

typedef struct {
    int      id;
    char     name[NAME_LEN];
    float    x, y;           /* screen position (used by raylib)  */
    NodeType type;
    Edge    *edges;          /* head of adjacency linked list     */
} Node;

typedef struct {
    Node nodes[MAX_NODES];
    int  count;
} Graph;

void  graph_init(Graph *g);
void  graph_free(Graph *g);

/* returns the new node id, or -1 on failure */
int   graph_add_node(Graph *g, const char *name, float x, float y, NodeType type);

/* adds a two-way road; returns 1 on success, 0 on failure */
int   graph_add_road(Graph *g, int u, int v, float distance);

/* finds the edge u -> v (NULL if there is no such road) */
Edge *graph_find_edge(const Graph *g, int u, int v);

/* these update BOTH directions of the road; return 1 on success */
int   graph_set_traffic(Graph *g, int u, int v, TrafficLevel level);
int   graph_set_closed (Graph *g, int u, int v, int closed);

/* how much slower a road is at this traffic level (1.0 / 1.5 / 2.5) */
float traffic_factor(TrafficLevel level);

/* cost of travelling along one edge (INF_COST if closed) */
float edge_cost(const Edge *e, CostMode mode);

void  graph_print(const Graph *g);

/* builds the fixed demo city used in the project */
void  graph_load_sample_map(Graph *g);

#endif
