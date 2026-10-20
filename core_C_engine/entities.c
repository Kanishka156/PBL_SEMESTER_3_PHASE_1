#include <stdio.h>
#include <math.h>
#include <string.h>
#include "entities.h"

const char *priority_name(Priority p)
{
    switch (p) {
        case PRI_CRITICAL: return "CRITICAL";
        case PRI_HIGH:     return "HIGH";
        default:           return "NORMAL";
    }
}

/* ============================================================
   Setup
   ============================================================ */

void sim_init(Sim *s, const Graph *g)
{
    memset(s, 0, sizeof(Sim));
    heap_init(&s->waiting);
    history_init(&s->history);
    s->nextEmId = 1;

    /* one ambulance parked at each hospital (up to MAX_AMBULANCES) */
    for (int i = 0; i < g->count && s->ambCount < MAX_AMBULANCES; i++) {
        if (g->nodes[i].type != NODE_HOSPITAL) continue;

        Ambulance *a = &s->amb[s->ambCount];
        a->id        = s->ambCount + 1;
        a->state     = AMB_AVAILABLE;
        a->node      = i;
        a->from      = i;
        a->to        = i;
        a->t         = 0.0f;
        a->target    = -1;
        a->emergency = -1;
        a->wait      = 0.0f;
        s->ambCount++;
    }
}

void sim_free(Sim *s)
{
    history_free(&s->history);
}

/* ============================================================
   Emergencies
   ============================================================ */

int sim_add_emergency(Sim *s, const Graph *g, int node, Priority p)
{
    if (node < 0 || node >= g->count) return -1;
    if (g->nodes[node].type != NODE_JUNCTION) return -1;   /* not at a hospital */

    /* find a free slot: a new one, or one whose emergency is finished */
    int slot = -1;
    if (s->emCount < MAX_EMERGENCIES) {
        slot = s->emCount++;
    } else {
        for (int i = 0; i < MAX_EMERGENCIES; i++) {
            if (s->em[i].status == EM_DONE) { slot = i; break; }
        }
    }
    if (slot == -1) return -1;                              /* too many at once */

    Emergency *em = &s->em[slot];
    em->id        = s->nextEmId++;
    em->node      = node;
    em->priority  = p;
    em->status    = EM_WAITING;
    em->ambulance = 0;
    em->created   = s->time;

    /* Heap key: priority first, then arrival order.
       CRITICAL (0) always comes out before HIGH (1) before NORMAL (2);
       inside the same priority, the older emergency (smaller id) goes first. */
    float key = (float)p * 1000.0f + (float)em->id;
    heap_push(&s->waiting, slot, key);

    return em->id;
}

int sim_waiting_count(const Sim *s)
{
    return s->waiting.size;
}

/* ============================================================
   Dispatch: most urgent emergency -> nearest free ambulance
   ============================================================ */

static int free_ambulance_exists(const Sim *s)
{
    for (int i = 0; i < s->ambCount; i++) {
        if (s->amb[i].state == AMB_AVAILABLE) return 1;
    }
    return 0;
}

static void dispatch(Sim *s, const Graph *g, CostMode mode)
{
    HeapItem skipped[MAX_EMERGENCIES];
    int nskipped = 0;
    HeapItem top;

    while (free_ambulance_exists(s) && heap_pop(&s->waiting, &top)) {
        Emergency *em = &s->em[top.node];

        /* one Dijkstra run from the emergency gives the distance to every ambulance */
        float dist[MAX_NODES];
        int   parent[MAX_NODES];
        dijkstra_run(g, em->node, mode, dist, parent);

        int best = -1;
        for (int i = 0; i < s->ambCount; i++) {
            if (s->amb[i].state != AMB_AVAILABLE) continue;
            float d = dist[s->amb[i].node];
            if (d >= INF_COST) continue;                    /* cut off by closed roads */
            if (best == -1 || d < dist[s->amb[best].node]) best = i;
        }

        if (best == -1) {
            /* no free ambulance can reach it right now: keep it waiting */
            if (nskipped < MAX_EMERGENCIES) skipped[nskipped++] = top;
            continue;
        }

        Ambulance *a = &s->amb[best];
        a->state     = AMB_TO_PATIENT;
        a->target    = em->node;
        a->emergency = top.node;
        em->status    = EM_ASSIGNED;
        em->ambulance = a->id;
    }

    for (int i = 0; i < nskipped; i++) {
        heap_push(&s->waiting, skipped[i].node, skipped[i].priority);
    }
}

/* ============================================================
   Driving
   ============================================================ */

static float segment_length(const Graph *g, int u, int v)
{
    float dx = g->nodes[v].x - g->nodes[u].x;
    float dy = g->nodes[v].y - g->nodes[u].y;
    float len = sqrtf(dx * dx + dy * dy);
    return len < 1.0f ? 1.0f : len;
}

/* the ambulance has reached its target node */
static void arrive(Sim *s, Ambulance *a)
{
    if (a->state == AMB_TO_PATIENT) {
        a->state = AMB_AT_PATIENT;
        a->wait  = PATIENT_WAIT;
    } else if (a->state == AMB_TO_HOSPITAL) {
        Emergency *em = &s->em[a->emergency];

        char note[NOTE_LEN];
        snprintf(note, sizeof(note), "%s, A%d", priority_name(em->priority), a->id);
        history_add(&s->history, em->node, a->node, s->time - em->created, note);

        em->status   = EM_DONE;
        a->emergency = -1;
        a->target    = -1;
        a->state     = AMB_AVAILABLE;      /* stays parked at this hospital */
    }
}

static void amb_step(Sim *s, const Graph *g, Ambulance *a, float dt, CostMode mode)
{
    if (a->state == AMB_AVAILABLE) return;

    if (a->state == AMB_AT_PATIENT) {
        a->wait -= dt;
        if (a->wait > 0.0f) return;

        /* loaded: pick the nearest hospital from here */
        PathResult h = find_nearest_hospital(g, a->node, mode);
        if (!h.found) {                    /* no way out right now: try again next frame */
            a->wait = 0.0f;
            return;
        }
        a->target = h.path[h.length - 1];
        a->state  = AMB_TO_HOSPITAL;
        return;
    }

    /* driving (TO_PATIENT or TO_HOSPITAL) */
    if (a->from == a->to) {                /* standing on a node */
        if (a->node == a->target) {
            arrive(s, a);
            return;
        }

        /* Plan the best route with the CURRENT map and take its first road.
           Because this is repeated at every junction, closed roads and new
           traffic are picked up automatically. */
        PathResult r = find_path(g, a->node, a->target, mode);
        if (!r.found || r.length < 2) return;       /* stuck, retry next frame */

        a->from = a->node;
        a->to   = r.path[1];
        a->t    = 0.0f;
    }

    /* drive along the current road; heavy traffic makes it slower */
    const Edge *e = graph_find_edge(g, a->from, a->to);
    float factor = (e != NULL) ? traffic_factor(e->traffic) : 1.0f;
    float len    = segment_length(g, a->from, a->to);

    a->t += dt * (AMB_SPEED / factor) / len;

    if (a->t >= 1.0f) {                    /* reached the next junction */
        a->node = a->to;
        a->from = a->to;
        a->t    = 0.0f;
        if (a->node == a->target) arrive(s, a);
    }
}

void sim_update(Sim *s, const Graph *g, float dt, CostMode mode)
{
    s->time += dt;
    dispatch(s, g, mode);
    for (int i = 0; i < s->ambCount; i++) {
        amb_step(s, g, &s->amb[i], dt, mode);
    }
}

/* ============================================================
   Helpers for drawing
   ============================================================ */

PathResult sim_plan(const Graph *g, const Ambulance *a, CostMode mode)
{
    PathResult none;
    none.found  = 0;
    none.cost   = INF_COST;
    none.length = 0;

    if (a->state != AMB_TO_PATIENT && a->state != AMB_TO_HOSPITAL) return none;

    /* if it is on a road, the route starts at the end of that road */
    int anchor = (a->from == a->to) ? a->node : a->to;
    return find_path(g, anchor, a->target, mode);
}

void amb_position(const Graph *g, const Ambulance *a, float *x, float *y)
{
    if (a->from == a->to) {
        *x = g->nodes[a->node].x;
        *y = g->nodes[a->node].y;
    } else {
        const Node *p = &g->nodes[a->from];
        const Node *q = &g->nodes[a->to];
        *x = p->x + (q->x - p->x) * a->t;
        *y = p->y + (q->y - p->y) * a->t;
    }
}
