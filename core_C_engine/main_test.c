#include <stdio.h>
#include "graph.h"
#include "heap.h"
#include "algorithms.h"

/* node ids of the sample map (see graph.c) */
#define CENTRAL_CHOWK   0
#define CLOCK_TOWER     1
#define RAILWAY_STATION 2
#define MARKET          4
#define UNIVERSITY      5
#define PARK_GATE       6
#define OLD_BRIDGE      7
#define CITY_HOSPITAL   10
#define METRO_HOSPITAL  11
#define GENERAL_HOSP    12

static void test_heap(void)
{
    printf("=== TEST 1: min-heap ===\n");
    MinHeap h;
    heap_init(&h);

    heap_push(&h, 1, 5.0f);
    heap_push(&h, 2, 1.0f);
    heap_push(&h, 3, 3.0f);
    heap_push(&h, 4, 0.5f);
    heap_push(&h, 5, 4.0f);

    printf("  Expected order of priorities: 0.5 1.0 3.0 4.0 5.0\n  Got:                          ");
    HeapItem it;
    while (heap_pop(&h, &it)) printf("%.1f ", it.priority);
    printf("\n\n");
}

static void show_both_modes(const Graph *g, int src, int dst)
{
    printf("  Shortest (distance only):\n");
    PathResult a = find_path(g, src, dst, COST_DISTANCE);
    path_print(g, &a);

    printf("  Fastest (with traffic):\n");
    PathResult b = find_path(g, src, dst, COST_TRAFFIC);
    path_print(g, &b);
}

int main(void)
{
    Graph g;
    graph_load_sample_map(&g);

    test_heap();

    printf("=== TEST 2: the sample city ===\n");
    graph_print(&g);
    printf("\n");

    printf("=== TEST 3: Railway Station -> Metro Hospital (no traffic) ===\n");
    show_both_modes(&g, RAILWAY_STATION, METRO_HOSPITAL);
    printf("\n");

    printf("=== TEST 4: heavy traffic on Clock Tower <-> Central Chowk ===\n");
    graph_set_traffic(&g, CLOCK_TOWER, CENTRAL_CHOWK, TRAFFIC_HEAVY);
    show_both_modes(&g, RAILWAY_STATION, METRO_HOSPITAL);
    printf("\n");

    printf("=== TEST 5: nearest hospital from Central Chowk ===\n");
    PathResult n1 = find_nearest_hospital(&g, CENTRAL_CHOWK, COST_TRAFFIC);
    path_print(&g, &n1);
    printf("\n");

    printf("=== TEST 6: road closed (University <-> Metro Hospital) ===\n");
    printf("  Before closure:\n");
    PathResult b1 = find_path(&g, UNIVERSITY, METRO_HOSPITAL, COST_TRAFFIC);
    path_print(&g, &b1);
    graph_set_closed(&g, UNIVERSITY, METRO_HOSPITAL, 1);
    printf("  After closure (automatic reroute):\n");
    PathResult b2 = find_path(&g, UNIVERSITY, METRO_HOSPITAL, COST_TRAFFIC);
    path_print(&g, &b2);
    printf("\n");

    printf("=== TEST 7: Metro Hospital completely cut off ===\n");
    graph_set_closed(&g, OLD_BRIDGE, METRO_HOSPITAL, 1);
    PathResult c = find_path(&g, RAILWAY_STATION, METRO_HOSPITAL, COST_TRAFFIC);
    path_print(&g, &c);
    printf("  Nearest hospital from Railway Station now:\n");
    PathResult c2 = find_nearest_hospital(&g, RAILWAY_STATION, COST_TRAFFIC);
    path_print(&g, &c2);
    printf("\n");

    printf("=== TEST 8: invalid input is handled safely ===\n");
    PathResult bad = find_path(&g, 99, 3, COST_DISTANCE);
    printf("  src=99 -> found = %d (expected 0)\n", bad.found);
    printf("  duplicate road added? %d (expected 0)\n",
           graph_add_road(&g, 0, 1, 1.0f));

    graph_free(&g);
    printf("\nAll tests finished.\n");
    return 0;
}
