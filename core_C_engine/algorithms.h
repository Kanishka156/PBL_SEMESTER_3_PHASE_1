#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include "graph.h"

/* ------------------------------------------------------------
   algorithms.h - Routing algorithms
   Dijkstra (best route), BFS (fewest roads / reachability),
   DFS (traversal / counting possible routes), alternate routes
   ------------------------------------------------------------ */

typedef struct {
    int   found;             /* 1 = a route exists, 0 = unreachable      */
    float cost;              /* total cost (km, traffic-weighted, or hops) */
    int   length;            /* number of nodes in path[]                */
    int   path[MAX_NODES];   /* node ids from source to destination      */
} PathResult;

/* ---------- Dijkstra ---------- */

/* Dijkstra from src to every node. Fills dist[] and parent[]
   (both arrays must have MAX_NODES entries). Closed roads are skipped. */
void dijkstra_run(const Graph *g, int src, CostMode mode,
                  float dist[], int parent[]);

/* Best route src -> dst */
PathResult find_path(const Graph *g, int src, int dst, CostMode mode);

/* Best route from src to the closest reachable hospital */
PathResult find_nearest_hospital(const Graph *g, int src, CostMode mode);

/* ---------- BFS ---------- */

/* Order in which BFS visits the nodes reachable from src (for animation).
   order[] must have MAX_NODES entries; *count receives how many were visited. */
void bfs_order(const Graph *g, int src, int order[], int *count);

/* Route with the FEWEST ROADS (ignores distance). cost = number of roads. */
PathResult find_fewest_roads_path(const Graph *g, int src, int dst);

/* 1 if dst can be reached from src using open roads, else 0 */
int is_reachable(const Graph *g, int src, int dst);

/* ---------- DFS ---------- */

/* Order in which DFS visits the nodes reachable from src (for animation). */
void dfs_order(const Graph *g, int src, int order[], int *count);

/* Number of different simple routes from src to dst using open roads
   (a measure of how many alternatives the city offers). */
int dfs_count_routes(const Graph *g, int src, int dst);

/* ---------- Alternate route ---------- */

/* Best route that avoids at least one road of the primary route.
   (Temporarily blocks each road of the primary route, one at a time,
   and keeps the cheapest result. The graph is restored afterwards.) */
PathResult find_alternate_path(Graph *g, int src, int dst, CostMode mode,
                               const PathResult *primary);

void path_print(const Graph *g, const PathResult *r);

#endif
