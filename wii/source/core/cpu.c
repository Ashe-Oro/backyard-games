#include "cpu.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* index.html roster, verbatim except typographic quotes/dashes mapped to ASCII (SPEC §2.6). */
const Rival byc_roster[BYC_RIVALS] = {
    {"Slidin' Steve", "The Patio Pusher", "Beanbag Bend", "2-7", "Your bags are in my driveway!", "slider", 0x45bca4, 0, 0},
    {"Big Bag Barb", "Queen of the Cookout", "Biscuit Bay", "5-3", "Hope you brought a bigger picnic!", "blocker", 0xed83bb, 0, 1},
    {"Kobe Kowalski", "The Sky Slinger", "Popcorn Plains", "9-4", "Forecast: bags from above!", "lobber", 0xf6b642, 1, 2},
    {"Sticky Vicky", "The Suede Sorceress", "Velcro Valley", "12-3", "This victory is going to stick!", "sniper", 0xa57cf0, 1, 3},
    {"Tailgate Ted", "The Parking Lot Tornado", "Grillville", "16-5", "I put the wild in wild card!", "wild", 0x73b5f3, 2, 4},
    {"Sierra Spin", "The Cyclone", "Corkscrew Creek", "24-6", "Catch my curve if you can!", "spinner", 0xf07832, 2, 5},
    {"Duke Dozer", "The Lane Clearer", "Granite Grove", "31-2", "Make way for the freight train!", "bulldozer", 0x8bd64b, 3, 6},
    {"Reign Supreme", "The Bag Baroness", "Crown Corners", "35-0", "The throne has no room for two!", "champion", 0xffe45e, 3, 7},
};
const char *const byc_circuits[4] = {"Backyard Circuit", "County Fair", "Tailgate Pro", "King's Court"};
const double byc_cpu_rates[BYC_RIVALS][2] = {{.07,.30},{.15,.35},{.25,.35},{.35,.35},{.45,.30},{.55,.30},{.67,.25},{.80,.15}};

static bool style(const Rival *r, const char *s) { return strcmp(r->style, s) == 0; }

CpuShot cpu_casual(const WinTable *win, const Wind *wind, Rng *rng) {
    CpuShot c = {0};
    c.side = rng_next(rng) < 0.3 ? SIDE_STICKY : SIDE_SLICK;
    c.arc = ARC_NORMAL; c.spin = 0; c.goal = GOAL_HOLE;
    const WinWindow *w = &win->w[1][c.side][ARC_NORMAL];
    c.power = w->mid + wind->z * 0.04 + rng_gauss(rng) * 0.055;
    c.aim = -wind->x * 0.08 + rng_gauss(rng) * 0.012;
    c.drift = rng_gauss(rng) * 0.18;
    return c;
}

void cpu_plan_begin(CpuPlan *p, const Rival *o, const Bag *bags, int n, const int scores[2], const Wind *wind) {
    memset(p, 0, sizeof(*p));
    p->rival = o; p->wind = *wind;
    const Vec3 hole = byc_board.hole_local;
    int blockers = 0; const Bag *own = NULL, *enemy = NULL;
    for (int i = 0; i < n; i++) {
        const Bag *b = &bags[i];
        if (!bag_on_deck(b)) continue;
        if (b->lp.z > hole.z + .12) blockers++;
        if (!own && b->team == 1 && hole_dist(b->lp) < .35) own = b;
        if (!enemy && b->team == 0) enemy = b;
    }
    BagArc arc = o->level == 0 ? ARC_NORMAL : style(o, "slider") ? ARC_SLIDER : style(o, "lobber") ? ARC_KOBE :
                 (style(o, "blocker") || style(o, "bulldozer")) ? ARC_SLIDER : ARC_NORMAL;
    if (o->level >= 3 && blockers && !style(o, "bulldozer")) arc = ARC_KOBE;
    BagSide side = (style(o, "sniper") || style(o, "blocker") || style(o, "bulldozer") || (style(o, "slider") && o->level > 0)) ? SIDE_STICKY : SIDE_SLICK;
    Vec3 target = hole; CpuGoal goal = GOAL_HOLE;
    if (o->level >= 2 && own && !style(o, "lobber")) { target = own->lp; arc = ARC_SLIDER; side = SIDE_SLICK; goal = GOAL_PUSH; }
    else if (o->level >= 4 && enemy && (scores[0] >= 18 || style(o, "bulldozer"))) { target = enemy->lp; arc = ARC_SLIDER; goal = GOAL_KNOCK; }
    else if (style(o, "blocker") && !blockers && scores[1] >= scores[0]) { target.z += .38; goal = GOAL_BLOCK; }
    p->arc = arc; p->side = side; p->goal = goal; p->target = target;
    p->curl = style(o, "spinner") ? .65 : 0;
    p->best_cost = INFINITY; p->best_power = .55; p->best_aim = 0;
    p->stage = PLAN_COARSE; p->power = .35; p->aim = -.06;
}

static void assess(CpuPlan *p, double power, double aim) {
    /* 1.0.3's step() reads the live round wind, so candidate flights include it. */
    Bag b; bag_new(&b, 1, p->side); bag_launch(&b, 0, power, aim, 0, p->arc, p->curl);
    for (int i = 0; i < 1920; i++) { bag_step(&b, &p->wind, 1.0 / 240); p->steps++; if (bag_is_final(&b)) break; }
    int k = b.state == BAG_IN ? 0 : b.state == BAG_BOARD ? 1 : 2;
    if (p->pool_n[k] < CPU_POOL_MAX) p->pool[k][p->pool_n[k]++] = (CpuCandidate){power, aim};
    if (b.state == BAG_IN && p->goal == GOAL_HOLE) { p->hole_power_sum += power; p->hole_aim_sum += aim; p->hole_shots++; }
    double dx = b.lp.x - p->target.x, dy = b.lp.y - p->target.y, dz = b.lp.z - p->target.z;
    double cost = sqrt(dx * dx + dy * dy + dz * dz);
    if (b.state == BAG_IN) cost = p->goal == GOAL_HOLE ? -2 + cost : .5;
    if (b.state == BAG_GRASS || b.state == BAG_REMOVED) cost += 3;
    if (cost < p->best_cost) { p->best_cost = cost; p->best_power = power; p->best_aim = aim; }
}

static double sort_best;
static int by_distance(const void *a, const void *b) {
    const CpuCandidate *x = a, *y = b;
    double dx = fabs(x->power - sort_best), dy = fabs(y->power - sort_best);
    if (dx < dy) return -1;
    if (dx > dy) return 1;
    return x < y ? -1 : x > y ? 1 : 0;   /* stable: JS Array.prototype.sort is stable */
}

static void finish(CpuPlan *p, Rng *rng) {
    const Rival *o = p->rival;
    double inside = byc_cpu_rates[o->level][0], on = byc_cpu_rates[o->level][1], roll = rng_next(rng);
    int k = roll < inside ? 0 : roll < inside + on ? 1 : 2;
    int n = p->pool_n[k];
    CpuCandidate best = {p->best_power, p->best_aim};
    if (n) {
        CpuCandidate *pool = p->pool[k];
        int choices = n;
        if (!style(o, "wild")) {
            sort_best = p->best_power;
            qsort(pool, (size_t)n, sizeof(*pool), by_distance);
            if (choices > 24) choices = 24;
        }
        best = pool[(int)floor(rng_next(rng) * choices)];
    }
    p->result = (CpuShot){p->side, p->arc, best.power, best.aim, 0, p->curl, p->goal};
    p->stage = PLAN_DONE;
}

bool cpu_plan_run(CpuPlan *p, long budget, Rng *rng) {
    long start = p->steps;
    while (p->stage != PLAN_DONE) {
        if (p->steps - start >= budget) return false;
        if (p->stage == PLAN_COARSE) {
            /* for(power=.35;power<=.85;power+=.0125) for(aim=-.06;aim<=.061;aim+=.012) */
            if (p->power <= .85) {
                if (p->aim <= .061) { assess(p, p->power, p->aim); p->aim += .012; continue; }
                p->power += .0125; p->aim = -.06; continue;
            }
            p->coarse_power = p->best_power; p->coarse_aim = p->best_aim;
            p->stage = PLAN_FINE; p->power = p->coarse_power - .015; p->aim = p->coarse_aim - .012;
        } else if (p->stage == PLAN_FINE) {
            if (p->power <= p->coarse_power + .015) {
                if (p->aim <= p->coarse_aim + .012) { assess(p, p->power, p->aim); p->aim += .002; continue; }
                p->power += .0025; p->aim = p->coarse_aim - .012; continue;
            }
            if (p->hole_shots) { p->best_power = p->hole_power_sum / p->hole_shots; p->best_aim = p->hole_aim_sum / p->hole_shots; }
            if (!p->pool_n[1] || !p->pool_n[0]) { p->stage = PLAN_FALLBACK; p->power = .08; p->aim = -.12; }
            else finish(p, rng);
        } else if (p->stage == PLAN_FALLBACK) {
            if (p->power <= .98) {
                if (p->aim <= .121) { assess(p, p->power, p->aim); p->aim += .02; continue; }
                p->power += .01; p->aim = -.12; continue;
            }
            finish(p, rng);
        }
    }
    return true;
}
