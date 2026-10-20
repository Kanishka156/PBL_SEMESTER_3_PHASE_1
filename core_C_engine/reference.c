/* reference.c - prints the answers of the C engine so the website can be compared against them.
   Build (Windows):  gcc reference.c graph.c heap.c algorithms.c stack.c queue.c -o reference.exe
   Run:              reference.exe > c_output.txt                                                  */
#include <stdio.h>
#include "graph.h"
#include "algorithms.h"

static void table(Graph *g, CostMode mode, const char *title)
{
    printf("%s\n", title);
    for (int s = 0; s < g->count; s++) {
        if (g->nodes[s].type == NODE_HOSPITAL) continue;

        PathResult main_route = find_nearest_hospital(g, s, mode);
        if (!main_route.found) {
            printf("%-16s -> NO ROUTE\n", g->nodes[s].name);
            continue;
        }
        int dest = main_route.path[main_route.length - 1];
        PathResult fewest = find_fewest_roads_path(g, s, dest);
        PathResult alt    = find_alternate_path(g, s, dest, mode, &main_route);

        printf("%-16s -> %-16s cost=%6.2f fewest=%d routes=%d ",
               g->nodes[s].name, g->nodes[dest].name, main_route.cost,
               (int)fewest.cost, dfs_count_routes(g, s, dest));
        if (alt.found) printf("alt=%.2f\n", alt.cost);
        else           printf("alt=none\n");
    }
    printf("\n");
}

int main(void)
{
    Graph g;
    graph_load_sample_map(&g);

    table(&g, COST_TRAFFIC, "SCENARIO 1: all roads clear, fastest mode");

    graph_set_closed(&g, 3, 4, 1);                 /* Bus Stand - Market closed   */
    graph_set_traffic(&g, 0, 1, TRAFFIC_HEAVY);    /* Central Chowk - Clock Tower heavy */
    table(&g, COST_TRAFFIC, "SCENARIO 2: Bus Stand-Market closed, Central Chowk-Clock Tower heavy, fastest mode");
    table(&g, COST_DISTANCE, "SCENARIO 3: same roads, shortest (distance only) mode");

    graph_free(&g);
    return 0;
}
