#ifndef ENTITIES_H
#define ENTITIES_H

#include "graph.h"
#include "heap.h"
#include "list.h"
#include "algorithms.h"

/* ------------------------------------------------------------
   entities.h - Ambulances, emergencies and the dispatch logic
   (no raylib in here: this is pure logic, like the other modules)
   ------------------------------------------------------------ */

#define MAX_AMBULANCES  3
#define MAX_EMERGENCIES 32
#define AMB_SPEED       130.0f   /* pixels per second on a clear road */
#define PATIENT_WAIT    1.0f     /* seconds spent loading a patient   */

/* smaller number = more urgent (the heap serves the smallest first) */
typedef enum { PRI_CRITICAL = 0, PRI_HIGH, PRI_NORMAL } Priority;

typedef enum {
    AMB_AVAILABLE = 0,   /* waiting at a node                    */
    AMB_TO_PATIENT,      /* driving to the emergency             */
    AMB_AT_PATIENT,      /* loading the patient                  */
    AMB_TO_HOSPITAL      /* driving the patient to a hospital    */
} AmbState;

typedef enum { EM_WAITING = 0, EM_ASSIGNED, EM_DONE } EmStatus;

typedef struct {
    int      id;          /* 1, 2, 3                                       */
    AmbState state;
    int      node;        /* the node it is standing on (when from == to)  */
    int      from, to;    /* the road it is driving on (equal when parked) */
    float    t;           /* progress along that road, 0.0 .. 1.0          */
    int      target;      /* node it is heading to (-1 = none)             */
    int      emergency;   /* slot of the emergency it serves (-1 = none)   */
    float    wait;        /* seconds left while loading the patient        */
} Ambulance;

typedef struct {
    int      id;          /* number shown to the user (1, 2, 3 ...)        */
    int      node;        /* where the emergency is                        */
    Priority priority;
    EmStatus status;
    int      ambulance;   /* id of the ambulance serving it (0 = none)     */
    float    created;     /* simulation time when it was reported          */
} Emergency;

typedef struct {
    Ambulance amb[MAX_AMBULANCES];
    int       ambCount;
    Emergency em[MAX_EMERGENCIES];
    int       emCount;
    int       nextEmId;
    MinHeap   waiting;      /* PRIORITY QUEUE of waiting emergencies */
    History   history;      /* LINKED LIST of completed dispatches   */
    float     time;         /* simulation clock (seconds)            */
} Sim;

void sim_init(Sim *s, const Graph *g);   /* parks one ambulance at each hospital */
void sim_free(Sim *s);

/* reports a new emergency at a junction; returns its id, or -1 if refused */
int  sim_add_emergency(Sim *s, const Graph *g, int node, Priority p);

/* advances the simulation by dt seconds (dispatching + driving) */
void sim_update(Sim *s, const Graph *g, float dt, CostMode mode);

/* the route an ambulance is currently following (found = 0 if parked) */
PathResult sim_plan(const Graph *g, const Ambulance *a, CostMode mode);

/* where to draw the ambulance (screen coordinates) */
void amb_position(const Graph *g, const Ambulance *a, float *x, float *y);

int  sim_waiting_count(const Sim *s);

const char *priority_name(Priority p);

#endif
