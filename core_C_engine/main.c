/* ------------------------------------------------------------
   main.c - Smart Ambulance Route and Traffic Optimizer (GUI)

   Step 2 of the GUI: ambulances, emergencies with priority,
   automatic dispatch, driving animation, status panel and history.
   ------------------------------------------------------------ */

#include <stddef.h>
#include <string.h>
#include <math.h>
#include "raylib.h"
#include "graph.h"
#include "algorithms.h"
#include "entities.h"

#define SCREEN_W 1200
#define SCREEN_H 680
#define PANEL_X  930          /* the info panel starts at this x   */
#define NODE_R   16           /* radius of a node circle           */

static const char *TRAFFIC_TEXT[3] = { "Clear", "Moderate", "Heavy" };

/* one colour per ambulance (A1, A2, A3) */
static const Color AMB_COLOR[MAX_AMBULANCES] = {
    { 130,  60, 200, 255 },
    {   0, 140, 140, 255 },
    { 200,  70, 130, 255 }
};

/* Everything the program needs to remember lives in this one struct */
typedef struct {
    Graph      g;
    Sim        sim;
    int        selNode;       /* selected location (-1 = none)           */
    int        selU, selV;    /* selected road (-1 = none)               */
    CostMode   mode;          /* shortest or traffic-aware               */
    Priority   newPriority;   /* priority given to the next emergency    */
    PathResult preview;       /* route from selected node to a hospital  */
} App;

/* ---------------- small helpers ---------------- */

static Vector2 node_pos(const Graph *g, int id)
{
    Vector2 p = { g->nodes[id].x, g->nodes[id].y };
    return p;
}

static Color with_alpha(Color c, unsigned char a)
{
    c.a = a;
    return c;
}

static void app_reset(App *a)
{
    sim_free(&a->sim);
    graph_free(&a->g);
    graph_load_sample_map(&a->g);
    sim_init(&a->sim, &a->g);

    a->selNode = -1;
    a->selU = -1;
    a->selV = -1;
    a->mode = COST_TRAFFIC;
    a->newPriority = PRI_NORMAL;
    a->preview.found = 0;
}

static Color traffic_color(const Edge *e)
{
    if (e->closed) return (Color){ 90, 90, 90, 255 };
    switch (e->traffic) {
        case TRAFFIC_CLEAR:    return (Color){ 46, 160, 67, 255 };
        case TRAFFIC_MODERATE: return (Color){ 240, 160, 30, 255 };
        default:               return (Color){ 210, 50, 50, 255 };
    }
}

static Color priority_color(Priority p)
{
    switch (p) {
        case PRI_CRITICAL: return (Color){ 220, 30, 30, 255 };
        case PRI_HIGH:     return (Color){ 245, 140, 20, 255 };
        default:           return (Color){ 230, 200, 40, 255 };
    }
}

/* distance from point p to the line segment a-b (used to click on roads) */
static float dist_point_segment(Vector2 p, Vector2 a, Vector2 b)
{
    float dx = b.x - a.x, dy = b.y - a.y;
    float len2 = dx * dx + dy * dy;
    float t = (len2 == 0.0f) ? 0.0f : ((p.x - a.x) * dx + (p.y - a.y) * dy) / len2;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    float cx = a.x + t * dx, cy = a.y + t * dy;
    float ex = p.x - cx,     ey = p.y - cy;
    return sqrtf(ex * ex + ey * ey);
}

/* which node is under the mouse? (-1 if none) */
static int pick_node(const Graph *g, Vector2 m)
{
    for (int i = 0; i < g->count; i++) {
        if (CheckCollisionPointCircle(m, node_pos(g, i), NODE_R + 4)) return i;
    }
    return -1;
}

/* which road is under the mouse? returns 1 and fills *u,*v if found */
static int pick_road(const Graph *g, Vector2 m, int *u, int *v)
{
    float best = 8.0f;                    /* must be within 8 pixels */
    int found = 0;

    for (int i = 0; i < g->count; i++) {
        for (Edge *e = g->nodes[i].edges; e != NULL; e = e->next) {
            if (e->to < i) continue;      /* each road is stored twice */
            float d = dist_point_segment(m, node_pos(g, i), node_pos(g, e->to));
            if (d < best) {
                best = d;
                *u = i;
                *v = e->to;
                found = 1;
            }
        }
    }
    return found;
}

/* ---------------- input ---------------- */

static void handle_input(App *a)
{
    Vector2 m = GetMousePosition();

    if (m.x < PANEL_X) {
        /* LEFT click: choose a location */
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            a->selNode = pick_node(&a->g, m);     /* -1 if clicked empty space */
        }
        /* RIGHT click: choose a road */
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            int u, v;
            if (pick_road(&a->g, m, &u, &v)) {
                a->selU = u;
                a->selV = v;
            } else {
                a->selU = -1;
                a->selV = -1;
            }
        }
    }

    if (IsKeyPressed(KEY_R)) {
        app_reset(a);
        return;
    }
    if (IsKeyPressed(KEY_M)) {
        a->mode = (a->mode == COST_DISTANCE) ? COST_TRAFFIC : COST_DISTANCE;
    }

    /* P: change the priority of the NEXT emergency */
    if (IsKeyPressed(KEY_P)) {
        a->newPriority = (Priority)((a->newPriority + 1) % 3);
    }

    /* E: report an emergency at the selected junction */
    if (IsKeyPressed(KEY_E) && a->selNode != -1) {
        sim_add_emergency(&a->sim, &a->g, a->selNode, a->newPriority);
    }

    /* G: report an emergency at a random junction with a random priority */
    if (IsKeyPressed(KEY_G)) {
        for (int tries = 0; tries < 50; tries++) {
            int n = GetRandomValue(0, a->g.count - 1);
            if (a->g.nodes[n].type != NODE_JUNCTION) continue;
            sim_add_emergency(&a->sim, &a->g, n, (Priority)GetRandomValue(0, 2));
            break;
        }
    }

    /* keys that work on the selected road */
    if (a->selU != -1) {
        if (IsKeyPressed(KEY_ONE))   graph_set_traffic(&a->g, a->selU, a->selV, TRAFFIC_CLEAR);
        if (IsKeyPressed(KEY_TWO))   graph_set_traffic(&a->g, a->selU, a->selV, TRAFFIC_MODERATE);
        if (IsKeyPressed(KEY_THREE)) graph_set_traffic(&a->g, a->selU, a->selV, TRAFFIC_HEAVY);

        if (IsKeyPressed(KEY_C)) {
            Edge *e = graph_find_edge(&a->g, a->selU, a->selV);
            if (e != NULL) graph_set_closed(&a->g, a->selU, a->selV, !e->closed);
        }
    }
}

/* ---------------- drawing the map ---------------- */

/* draws a route as one wide line under the roads */
static void draw_route_line(const Graph *g, const PathResult *r, float width, Color c)
{
    for (int i = 0; i < r->length - 1; i++) {
        DrawLineEx(node_pos(g, r->path[i]), node_pos(g, r->path[i + 1]), width, c);
    }
}

static void draw_routes(const App *a)
{
    const Graph *g = &a->g;

    /* gold: preview from the selected location to its nearest hospital */
    if (a->preview.found) draw_route_line(g, &a->preview, 11.0f, GOLD);

    /* each driving ambulance shows its own route in its own colour */
    for (int i = 0; i < a->sim.ambCount; i++) {
        const Ambulance *amb = &a->sim.amb[i];
        PathResult r = sim_plan(g, amb, a->mode);
        if (!r.found) continue;

        float x, y;
        amb_position(g, amb, &x, &y);
        Vector2 here = { x, y };
        DrawLineEx(here, node_pos(g, r.path[0]), 7.0f, with_alpha(AMB_COLOR[i], 170));
        draw_route_line(g, &r, 7.0f, with_alpha(AMB_COLOR[i], 170));
    }
}

static void draw_roads(const App *a)
{
    const Graph *g = &a->g;

    for (int i = 0; i < g->count; i++) {
        for (Edge *e = g->nodes[i].edges; e != NULL; e = e->next) {
            if (e->to < i) continue;              /* draw each road once */

            Vector2 p = node_pos(g, i);
            Vector2 q = node_pos(g, e->to);

            /* purple halo around the selected road */
            if (i == a->selU && e->to == a->selV) {
                DrawLineEx(p, q, 9.0f, (Color){ 160, 100, 255, 255 });
            }

            DrawLineEx(p, q, 4.0f, traffic_color(e));

            /* distance label in the middle of the road */
            int mx = (int)((p.x + q.x) / 2);
            int my = (int)((p.y + q.y) / 2);
            DrawText(TextFormat("%.1f", e->distance), mx - 10, my - 18, 14, DARKGRAY);

            if (e->closed) {
                DrawText("X", mx - 6, my - 6, 22, RED);
            }
        }
    }
}

static void draw_nodes(const App *a, int hover)
{
    const Graph *g = &a->g;

    for (int i = 0; i < g->count; i++) {
        const Node *n = &g->nodes[i];
        int x = (int)n->x, y = (int)n->y;

        Color fill = (n->type == NODE_HOSPITAL) ? (Color){ 200, 30, 50, 255 }
                                                : (Color){ 40, 100, 220, 255 };
        DrawCircle(x, y, NODE_R, fill);
        DrawCircleLines(x, y, NODE_R, BLACK);

        if (n->type == NODE_HOSPITAL) {
            DrawText("H", x - 6, y - 10, 20, WHITE);
        }

        if (i == hover) DrawCircleLines(x, y, NODE_R + 4, DARKGRAY);
        if (i == a->selNode) {
            DrawCircleLines(x, y, NODE_R + 5, ORANGE);
            DrawCircleLines(x, y, NODE_R + 6, ORANGE);
        }

        int w = MeasureText(n->name, 14);
        DrawText(n->name, x - w / 2, y + NODE_R + 5, 14, BLACK);
    }
}

/* a round "!" marker next to every active emergency, coloured by priority */
static void draw_emergencies(const App *a)
{
    for (int i = 0; i < a->sim.emCount; i++) {
        const Emergency *em = &a->sim.em[i];
        if (em->status == EM_DONE) continue;

        const Node *n = &a->g.nodes[em->node];
        int x = (int)n->x + NODE_R + 8;
        int y = (int)n->y - NODE_R - 8;

        /* waiting emergencies pulse, assigned ones stay still */
        float r = 10.0f;
        if (em->status == EM_WAITING) r += 2.5f * sinf((float)GetTime() * 7.0f);

        DrawCircle(x, y, r, priority_color(em->priority));
        DrawCircleLines(x, y, r, BLACK);
        DrawText("!", x - 3, y - 8, 16, BLACK);
    }
}

static void draw_ambulances(const App *a)
{
    for (int i = 0; i < a->sim.ambCount; i++) {
        const Ambulance *amb = &a->sim.amb[i];

        float fx, fy;
        amb_position(&a->g, amb, &fx, &fy);
        int x = (int)fx, y = (int)fy;

        /* parked ambulances are drawn above their node so they don't hide it */
        if (amb->from == amb->to) {
            x += (amb->id - 2) * 30;
            y -= 36;
        }

        DrawRectangle(x - 15, y - 10, 30, 20, AMB_COLOR[i]);
        DrawRectangleLines(x - 15, y - 10, 30, 20, BLACK);
        DrawText(TextFormat("A%d", amb->id), x - 9, y - 7, 15, WHITE);
    }
}

/* ---------------- side panel ---------------- */

/* writes one line of text and moves the y cursor down */
static void put(int x, int *y, int size, Color c, const char *text)
{
    DrawText(text, x, *y, size, c);
    *y += size + 4;
}

static const char *amb_status_text(const App *a, const Ambulance *amb)
{
    const Graph *g = &a->g;
    switch (amb->state) {
        case AMB_AVAILABLE:
            return TextFormat("Free @ %s", g->nodes[amb->node].name);
        case AMB_TO_PATIENT:
            return TextFormat("To patient: %s", g->nodes[amb->target].name);
        case AMB_AT_PATIENT:
            return amb->wait > 0.0f ? "Loading patient" : "NO ROUTE to hospital";
        default:
            return TextFormat("To hospital: %s", g->nodes[amb->target].name);
    }
}

static void draw_panel(const App *a)
{
    const Graph *g = &a->g;
    int x = PANEL_X + 12;
    int y = 10;

    DrawRectangle(PANEL_X, 0, SCREEN_W - PANEL_X, SCREEN_H, (Color){ 235, 238, 245, 255 });
    DrawLine(PANEL_X, 0, PANEL_X, SCREEN_H, GRAY);

    put(x, &y, 20, DARKBLUE, "CONTROL PANEL");
    y += 4;

    put(x, &y, 14, BLACK, TextFormat("Route mode (M): %s",
        a->mode == COST_DISTANCE ? "Shortest" : "Fastest"));
    put(x, &y, 14, priority_color(a->newPriority),
        TextFormat("Next emergency (P): %s", priority_name(a->newPriority)));
    y += 4;

    /* selected location + preview */
    if (a->selNode == -1) {
        put(x, &y, 14, GRAY, "Location: (left-click a junction)");
    } else {
        put(x, &y, 14, BLACK, TextFormat("Location: %s", g->nodes[a->selNode].name));
        if (a->preview.found) {
            int dest = a->preview.path[a->preview.length - 1];
            put(x, &y, 13, MAROON, TextFormat("Nearest hospital: %s", g->nodes[dest].name));
            put(x, &y, 13, DARKGRAY, TextFormat("Cost %.1f, %d roads",
                a->preview.cost, a->preview.length - 1));
        } else {
            put(x, &y, 13, RED, "NO ROUTE to any hospital");
        }
    }
    y += 4;

    /* selected road */
    if (a->selU == -1) {
        put(x, &y, 14, GRAY, "Road: (right-click a road)");
    } else {
        const Edge *e = graph_find_edge(g, a->selU, a->selV);
        put(x, &y, 13, BLACK, TextFormat("%s <-> %s",
            g->nodes[a->selU].name, g->nodes[a->selV].name));
        if (e != NULL) {
            put(x, &y, 13, BLACK, TextFormat("%.1f km, traffic: %s",
                e->distance, TRAFFIC_TEXT[e->traffic]));
            put(x, &y, 13, e->closed ? RED : DARKGREEN,
                e->closed ? "Status: CLOSED" : "Status: open");
        }
    }
    y += 8;

    /* ambulances */
    put(x, &y, 16, DARKBLUE, "AMBULANCES");
    for (int i = 0; i < a->sim.ambCount; i++) {
        const Ambulance *amb = &a->sim.amb[i];
        DrawRectangle(x, y + 1, 11, 11, AMB_COLOR[i]);
        DrawText(TextFormat("A%d  %s", amb->id, amb_status_text(a, amb)),
                 x + 17, y, 13, BLACK);
        y += 18;
    }
    y += 2;
    put(x, &y, 13, DARKGRAY, TextFormat("Emergencies waiting: %d", sim_waiting_count(&a->sim)));
    put(x, &y, 13, DARKGRAY, TextFormat("Completed: %d", a->sim.history.count));
    y += 6;

    /* history: the last three completed dispatches */
    put(x, &y, 16, DARKBLUE, "HISTORY (latest)");
    if (a->sim.history.count == 0) {
        put(x, &y, 13, GRAY, "(none yet)");
    } else {
        int first = a->sim.history.count - 3;
        if (first < 0) first = 0;
        for (int i = a->sim.history.count - 1; i >= first; i--) {
            const HistoryNode *h = history_get(&a->sim.history, i);
            if (h == NULL) continue;
            put(x, &y, 13, BLACK, TextFormat("#%d %s > %s",
                h->id, g->nodes[h->from_node].name, g->nodes[h->to_node].name));
            put(x, &y, 12, DARKGRAY, TextFormat("    %.1f s  [%s]", h->cost, h->note));
        }
    }
    y += 8;

    put(x, &y, 16, DARKBLUE, "CONTROLS");
    put(x, &y, 13, DARKGRAY, "Left-click: location  Right-click: road");
    put(x, &y, 13, DARKGRAY, "E: emergency here   G: random emergency");
    put(x, &y, 13, DARKGRAY, "P: priority         M: route mode");
    put(x, &y, 13, DARKGRAY, "1/2/3: traffic      C: close/open road");
    put(x, &y, 13, DARKGRAY, "R: reset            Esc: quit");
}

/* ---------------- main ---------------- */

int main(void)
{
    App app;
    memset(&app, 0, sizeof(app));       /* everything starts empty / NULL / 0 */
    app_reset(&app);

    InitWindow(SCREEN_W, SCREEN_H, "Smart Ambulance Route and Traffic Optimizer");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.1f) dt = 0.1f;       /* avoid big jumps if the window lags */

        /* 1. read the mouse and keyboard */
        handle_input(&app);

        /* 2. advance the simulation: dispatch + ambulances driving */
        sim_update(&app.sim, &app.g, dt, app.mode);

        /* 3. preview route for the selected location */
        if (app.selNode != -1) {
            app.preview = find_nearest_hospital(&app.g, app.selNode, app.mode);
        } else {
            app.preview.found = 0;
        }

        /* 4. draw everything */
        int hover = pick_node(&app.g, GetMousePosition());

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText("Smart Ambulance Route and Traffic Optimizer", 20, 15, 24, DARKBLUE);
        draw_routes(&app);
        draw_roads(&app);
        draw_nodes(&app, hover);
        draw_emergencies(&app);
        draw_ambulances(&app);
        draw_panel(&app);

        /* small legend at the bottom-left */
        DrawText("Road colour:", 20, SCREEN_H - 30, 16, DARKGRAY);
        DrawRectangle(125, SCREEN_H - 28, 14, 14, (Color){ 46, 160, 67, 255 });
        DrawText("clear", 145, SCREEN_H - 30, 16, DARKGRAY);
        DrawRectangle(205, SCREEN_H - 28, 14, 14, (Color){ 240, 160, 30, 255 });
        DrawText("moderate", 225, SCREEN_H - 30, 16, DARKGRAY);
        DrawRectangle(315, SCREEN_H - 28, 14, 14, (Color){ 210, 50, 50, 255 });
        DrawText("heavy", 335, SCREEN_H - 30, 16, DARKGRAY);
        DrawRectangle(405, SCREEN_H - 28, 14, 14, (Color){ 90, 90, 90, 255 });
        DrawText("closed", 425, SCREEN_H - 30, 16, DARKGRAY);

        EndDrawing();
    }

    CloseWindow();
    sim_free(&app.sim);
    graph_free(&app.g);
    return 0;
}
