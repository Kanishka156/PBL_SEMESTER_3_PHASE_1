#include <stdio.h>
#include "graph.h"
#include "stack.h"
#include "queue.h"
#include "list.h"
#include "algorithms.h"

/* node ids of the sample map (see graph.c) */
#define CENTRAL_CHOWK   0
#define RAILWAY_STATION 2
#define UNIVERSITY      5
#define OLD_BRIDGE      7
#define CITY_HOSPITAL   10
#define METRO_HOSPITAL  11
#define GENERAL_HOSP    12

static void print_order(const Graph *g, const int order[], int count)
{
    printf("    ");
    for (int i = 0; i < count; i++) {
        printf("%s%s", g->nodes[order[i]].name, i < count - 1 ? ", " : "");
    }
    printf("\n    (%d nodes visited)\n", count);
}

static void test_stack(void)
{
    printf("=== TEST 1: stack (LIFO) ===\n");
    IntStack s;
    stack_init(&s);
    stack_push(&s, 1);
    stack_push(&s, 2);
    stack_push(&s, 3);

    int v;
    printf("  Expected pop order: 3 2 1\n  Got:                ");
    while (stack_pop(&s, &v)) printf("%d ", v);
    printf("\n  Pop on empty stack returns %d (expected 0)\n\n",
           stack_pop(&s, &v));
}

static void test_queue(void)
{
    printf("=== TEST 2: queue (FIFO, circular) ===\n");
    IntQueue q;
    queue_init(&q);
    enqueue(&q, 10);
    enqueue(&q, 20);
    enqueue(&q, 30);

    int v;
    printf("  Expected order: 10 20 30\n  Got:            ");
    while (dequeue(&q, &v)) printf("%d ", v);

    /* push far more items than MAX_QUEUE overall to test wrap-around */
    int ok = 1;
    for (int i = 0; i < 200; i++) {
        enqueue(&q, i);
        dequeue(&q, &v);
        if (v != i) ok = 0;
    }
    printf("\n  Wrap-around test (200 items through a %d-slot queue): %s\n",
           MAX_QUEUE, ok ? "PASS" : "FAIL");

    for (int i = 0; i < MAX_QUEUE; i++) enqueue(&q, i);
    printf("  Enqueue on a full queue returns %d (expected 0)\n\n",
           enqueue(&q, 999));
}

static void test_history(const Graph *g)
{
    printf("=== TEST 3: linked list (dispatch history) ===\n");
    History h;
    history_init(&h);

    history_add(&h, RAILWAY_STATION, METRO_HOSPITAL, 9.20f, "critical");
    history_add(&h, CENTRAL_CHOWK,   CITY_HOSPITAL,  4.60f, "normal");
    history_add(&h, UNIVERSITY,      METRO_HOSPITAL, 4.20f, "rerouted");

    history_print(&h, g);
    printf("  Records stored: %d (expected 3)\n", h.count);

    const HistoryNode *second = history_get(&h, 1);
    printf("  Record at index 1 has id %d (expected 2)\n",
           second ? second->id : -1);
    printf("  Record at index 9 is %s (expected NULL)\n",
           history_get(&h, 9) == NULL ? "NULL" : "NOT NULL");

    history_free(&h);
    printf("  After reset, records: %d (expected 0)\n\n", h.count);
}

int main(void)
{
    Graph g;
    graph_load_sample_map(&g);

    test_stack();
    test_queue();
    test_history(&g);

    int order[MAX_NODES], count;

    printf("=== TEST 4: BFS visiting order from Central Chowk ===\n");
    bfs_order(&g, CENTRAL_CHOWK, order, &count);
    print_order(&g, order, count);
    printf("\n");

    printf("=== TEST 5: DFS visiting order from Central Chowk ===\n");
    dfs_order(&g, CENTRAL_CHOWK, order, &count);
    print_order(&g, order, count);
    printf("\n");

    printf("=== TEST 6: fewest roads vs Dijkstra (Railway Station -> General Hospital) ===\n");
    printf("  BFS (fewest roads):\n");
    PathResult bfs = find_fewest_roads_path(&g, RAILWAY_STATION, GENERAL_HOSP);
    path_print(&g, &bfs);
    printf("  Dijkstra (shortest distance):\n");
    PathResult dij = find_path(&g, RAILWAY_STATION, GENERAL_HOSP, COST_DISTANCE);
    path_print(&g, &dij);
    printf("\n");

    printf("=== TEST 7: how many routes exist? (DFS counting) ===\n");
    printf("  Railway Station -> Metro Hospital: %d routes\n",
           dfs_count_routes(&g, RAILWAY_STATION, METRO_HOSPITAL));
    graph_set_closed(&g, UNIVERSITY, METRO_HOSPITAL, 1);
    printf("  after closing University <-> Metro Hospital: %d routes\n",
           dfs_count_routes(&g, RAILWAY_STATION, METRO_HOSPITAL));
    graph_set_closed(&g, UNIVERSITY, METRO_HOSPITAL, 0);
    printf("\n");

    printf("=== TEST 8: alternate route (Railway Station -> Metro Hospital) ===\n");
    PathResult main_route = find_path(&g, RAILWAY_STATION, METRO_HOSPITAL, COST_DISTANCE);
    printf("  Main route:\n");
    path_print(&g, &main_route);
    PathResult alt = find_alternate_path(&g, RAILWAY_STATION, METRO_HOSPITAL,
                                         COST_DISTANCE, &main_route);
    printf("  Alternate route:\n");
    path_print(&g, &alt);

    PathResult again = find_path(&g, RAILWAY_STATION, METRO_HOSPITAL, COST_DISTANCE);
    printf("  Graph restored after search? %s\n\n",
           (again.found && again.cost == main_route.cost) ? "yes" : "NO");

    printf("=== TEST 9: reachability after a blockade ===\n");
    printf("  Metro Hospital reachable from Railway Station: %d (expected 1)\n",
           is_reachable(&g, RAILWAY_STATION, METRO_HOSPITAL));
    graph_set_closed(&g, OLD_BRIDGE, METRO_HOSPITAL, 1);
    graph_set_closed(&g, UNIVERSITY, METRO_HOSPITAL, 1);
    printf("  after blocking both roads to it:               %d (expected 0)\n",
           is_reachable(&g, RAILWAY_STATION, METRO_HOSPITAL));
    PathResult none = find_alternate_path(&g, RAILWAY_STATION, METRO_HOSPITAL,
                                          COST_DISTANCE, &(PathResult){0});
    printf("  alternate path when no main route: found = %d (expected 0)\n\n",
           none.found);

    printf("=== TEST 10: invalid input is handled safely ===\n");
    bfs_order(&g, 99, order, &count);
    printf("  BFS from node 99 visited %d nodes (expected 0)\n", count);
    printf("  DFS routes with bad node: %d (expected 0)\n",
           dfs_count_routes(&g, -1, 3));

    graph_free(&g);
    printf("\nAll tests finished.\n");
    return 0;
}
