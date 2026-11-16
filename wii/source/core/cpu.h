/* CPU opponents: ports of index.html nextTurn's casual branch and planCareerShot().
 * The career planner is a resumable job so the Wii never blocks a frame (SPEC §2.4). It keeps the
 * 1.0.3 single-bag step()-only predictor (no collide() against blockers), on purpose. */
#ifndef BYC_CPU_H
#define BYC_CPU_H

#include "physics.h"
#include "rng.h"

typedef struct {
    const char *name, *nick, *home, *record, *taunt, *style;
    unsigned color;   /* 0xRRGGBB */
    int circuit, level;
} Rival;
#define BYC_RIVALS 8
extern const Rival byc_roster[BYC_RIVALS];
extern const char *const byc_circuits[4];
extern const double byc_cpu_rates[BYC_RIVALS][2];

typedef struct { double power, aim; } CpuCandidate;
typedef enum { GOAL_HOLE, GOAL_PUSH, GOAL_KNOCK, GOAL_BLOCK } CpuGoal;
typedef struct {
    BagSide side;
    BagArc arc;
    double power, aim, drift, spin;
    CpuGoal goal;
} CpuShot;

#define CPU_POOL_MAX 2048
typedef enum { PLAN_COARSE, PLAN_FINE, PLAN_FALLBACK, PLAN_DONE } PlanStage;
typedef struct {
    const Rival *rival;
    Wind wind;
    PlanStage stage;
    BagArc arc; BagSide side; CpuGoal goal; double curl;
    Vec3 target;
    double best_cost, best_power, best_aim;
    double coarse_power, coarse_aim;
    double power, aim;            /* current loop counters (JS float accumulation reproduced) */
    double hole_power_sum, hole_aim_sum; int hole_shots;
    CpuCandidate pool[3][CPU_POOL_MAX]; int pool_n[3];  /* in, board, miss */
    long steps;                   /* physics steps simulated (benchmark) */
    CpuShot result;
} CpuPlan;

/* Casual CPU (nextTurn): consumes 1 + 3 gauss draws like 1.0.3 even before a career plan. */
CpuShot cpu_casual(const WinTable *win, const Wind *wind, Rng *rng);
/* Starts planCareerShot for the current deck. bags/n: every bag in the round; scores: match. */
void cpu_plan_begin(CpuPlan *plan, const Rival *rival, const Bag *bags, int n, const int scores[2], const Wind *wind);
/* Runs candidate simulations until about step_budget physics steps have run. True when finished
 * (then plan->result is valid, and the CPU_RATES pool draw has consumed rng). */
bool cpu_plan_run(CpuPlan *plan, long step_budget, Rng *rng);

#endif
