#include <stdio.h>
#include "graph.h"
#include "entities.h"

/* node ids of the sample map (see graph.c) */
#define CENTRAL_CHOWK   0
#define CLOCK_TOWER     1
#define RAILWAY_STATION 2
#define BUS_STAND       3
#define MARKET          4
#define UNIVERSITY      5
#define RING_NORTH      8
#define CITY_HOSPITAL   10

static int failures = 0;

#define CHECK(cond, text) do {                                   \
    if (cond) printf("  PASS  %s\n", text);                      \
    else { printf("  FAIL  %s\n", text); failures++; }           \
} while (0)

/* runs the simulation until every emergency is done (or time runs out) */
static int run_until_done(Sim *s, const Graph *g, CostMode mode, float max_seconds)
{
    float t = 0.0f;
    while (t < max_seconds) {
        sim_update(s, g, 0.05f, mode);
        t += 0.05f;

        int open = 0;
        for (int i = 0; i < s->emCount; i++) {
            if (s->em[i].status != EM_DONE) open++;
        }
        if (open == 0) return 1;
    }
    return 0;
}

static int find_em(const Sim *s, int id)
{
    for (int i = 0; i < s->emCount; i++) if (s->em[i].id == id) return i;
    return -1;
}

static void test_setup(void)
{
    printf("=== TEST 1: setup ===\n");
    Graph g; graph_load_sample_map(&g);
    Sim s;   sim_init(&s, &g);

    CHECK(s.ambCount == 3, "3 ambulances created");
    CHECK(s.amb[0].node == 10 && s.amb[1].node == 11 && s.amb[2].node == 12,
          "one ambulance parked at each hospital");
    CHECK(s.amb[0].state == AMB_AVAILABLE, "ambulances start available");
    CHECK(sim_add_emergency(&s, &g, 10, PRI_NORMAL) == -1, "emergency at a hospital is refused");
    CHECK(sim_add_emergency(&s, &g, 99, PRI_NORMAL) == -1, "emergency at an invalid node is refused");
    CHECK(sim_add_emergency(&s, &g, MARKET, PRI_NORMAL) == 1, "valid emergency gets id 1");

    sim_free(&s); graph_free(&g);
    printf("\n");
}

static void test_priority_order(void)
{
    printf("=== TEST 2: most urgent emergencies are served first ===\n");
    Graph g; graph_load_sample_map(&g);
    Sim s;   sim_init(&s, &g);

    /* five emergencies reported BEFORE any ambulance moves */
    int e1 = sim_add_emergency(&s, &g, BUS_STAND,       PRI_NORMAL);    /* id 1 */
    int e2 = sim_add_emergency(&s, &g, RING_NORTH,      PRI_HIGH);      /* id 2 */
    int e3 = sim_add_emergency(&s, &g, MARKET,          PRI_NORMAL);    /* id 3 */
    int e4 = sim_add_emergency(&s, &g, RAILWAY_STATION, PRI_CRITICAL);  /* id 4 */
    int e5 = sim_add_emergency(&s, &g, UNIVERSITY,      PRI_HIGH);      /* id 5 */

    sim_update(&s, &g, 0.05f, COST_TRAFFIC);

    CHECK(s.em[find_em(&s, e4)].status == EM_ASSIGNED, "CRITICAL (Railway Station) dispatched first");
    CHECK(s.em[find_em(&s, e2)].status == EM_ASSIGNED, "HIGH (Ring Road North) dispatched");
    CHECK(s.em[find_em(&s, e5)].status == EM_ASSIGNED, "HIGH (University) dispatched");
    CHECK(s.em[find_em(&s, e1)].status == EM_WAITING,  "NORMAL (Bus Stand) has to wait");
    CHECK(s.em[find_em(&s, e3)].status == EM_WAITING,  "NORMAL (Market) has to wait");
    CHECK(sim_waiting_count(&s) == 2, "2 emergencies waiting in the priority queue");

    /* among equal priority, the OLDER one (Bus Stand, id 1) goes first */
    int first_assigned = -1;
    for (int step = 0; step < 6000 && first_assigned == -1; step++) {
        sim_update(&s, &g, 0.05f, COST_TRAFFIC);
        if (s.em[find_em(&s, e1)].status != EM_WAITING) first_assigned = e1;
        else if (s.em[find_em(&s, e3)].status != EM_WAITING) first_assigned = e3;
    }
    CHECK(first_assigned == e1, "between two NORMAL emergencies, the older one is served first");

    CHECK(run_until_done(&s, &g, COST_TRAFFIC, 600.0f), "all 5 emergencies are completed");
    CHECK(s.history.count == 5, "history contains 5 records");
    CHECK(sim_waiting_count(&s) == 0, "nobody left waiting");

    sim_free(&s); graph_free(&g);
    printf("\n");
}

static void test_unreachable(void)
{
    printf("=== TEST 3: emergency that nobody can reach ===\n");
    Graph g; graph_load_sample_map(&g);
    Sim s;   sim_init(&s, &g);

    /* cut Railway Station off completely: roads to Bus Stand and Clock Tower */
    graph_set_closed(&g, RAILWAY_STATION, BUS_STAND, 1);
    graph_set_closed(&g, RAILWAY_STATION, CLOCK_TOWER, 1);

    sim_add_emergency(&s, &g, RAILWAY_STATION, PRI_CRITICAL);
    for (int i = 0; i < 100; i++) sim_update(&s, &g, 0.05f, COST_TRAFFIC);

    CHECK(sim_waiting_count(&s) == 1, "cut-off emergency keeps waiting");
    CHECK(s.amb[0].state == AMB_AVAILABLE && s.amb[1].state == AMB_AVAILABLE &&
          s.amb[2].state == AMB_AVAILABLE, "no ambulance is sent on an impossible trip");

    /* a normal emergency elsewhere is NOT blocked by the impossible one */
    sim_add_emergency(&s, &g, MARKET, PRI_NORMAL);
    for (int i = 0; i < 20; i++) sim_update(&s, &g, 0.05f, COST_TRAFFIC);
    CHECK(sim_waiting_count(&s) == 1, "other emergencies are still dispatched");

    /* reopen one road: the waiting emergency is dispatched */
    graph_set_closed(&g, RAILWAY_STATION, BUS_STAND, 0);
    CHECK(run_until_done(&s, &g, COST_TRAFFIC, 600.0f), "after reopening a road, everything is completed");

    sim_free(&s); graph_free(&g);
    printf("\n");
}

static void test_reroute_while_driving(void)
{
    printf("=== TEST 4: road closes while the ambulance is driving ===\n");
    Graph g; graph_load_sample_map(&g);
    Sim s;   sim_init(&s, &g);

    sim_add_emergency(&s, &g, RAILWAY_STATION, PRI_CRITICAL);

    /* drive for a while, then block the roads near the ambulance */
    for (int i = 0; i < 60; i++) sim_update(&s, &g, 0.05f, COST_TRAFFIC);
    graph_set_closed(&g, BUS_STAND, MARKET, 1);
    graph_set_closed(&g, CLOCK_TOWER, CENTRAL_CHOWK, 1);

    CHECK(run_until_done(&s, &g, COST_TRAFFIC, 600.0f), "emergency still completes after closures");
    CHECK(s.history.count == 1, "one history record");

    sim_free(&s); graph_free(&g);
    printf("\n");
}

static float trip_time(TrafficLevel level)
{
    Graph g; graph_load_sample_map(&g);
    Sim s;   sim_init(&s, &g);

    graph_set_traffic(&g, MARKET, CITY_HOSPITAL, level);
    sim_add_emergency(&s, &g, MARKET, PRI_NORMAL);
    /* COST_DISTANCE: the ambulance ignores traffic when planning, so it
       keeps using this road and the only difference is its driving speed */
    run_until_done(&s, &g, COST_DISTANCE, 600.0f);

    float t = (s.history.count > 0) ? s.history.head->cost : -1.0f;
    sim_free(&s); graph_free(&g);
    return t;
}

static void test_traffic_slows_driving(void)
{
    printf("=== TEST 5: traffic slows the ambulance down ===\n");
    float clear = trip_time(TRAFFIC_CLEAR);
    float heavy = trip_time(TRAFFIC_HEAVY);
    printf("  trip with clear road: %.1f s,  with heavy traffic: %.1f s\n", clear, heavy);
    CHECK(clear > 0.0f && heavy > clear * 1.5f, "heavy traffic makes the trip clearly slower");
    printf("\n");
}

static void test_limits(void)
{
    printf("=== TEST 6: too many emergencies ===\n");
    Graph g; graph_load_sample_map(&g);
    Sim s;   sim_init(&s, &g);

    int accepted = 0;
    for (int i = 0; i < 40; i++) {
        if (sim_add_emergency(&s, &g, MARKET, PRI_NORMAL) != -1) accepted++;
    }
    CHECK(accepted == MAX_EMERGENCIES, "no more than MAX_EMERGENCIES accepted at once");
    CHECK(run_until_done(&s, &g, COST_TRAFFIC, 5000.0f), "all accepted emergencies get completed");
    CHECK(sim_add_emergency(&s, &g, MARKET, PRI_NORMAL) != -1, "finished slots are reused");

    sim_free(&s); graph_free(&g);
    printf("\n");
}

int main(void)
{
    test_setup();
    test_priority_order();
    test_unreachable();
    test_reroute_while_driving();
    test_traffic_slows_driving();
    test_limits();

    printf(failures == 0 ? "ALL TESTS PASSED\n" : "%d TEST(S) FAILED\n", failures);
    return failures;
}
