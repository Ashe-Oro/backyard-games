/* Backyard Cornhole Wii — bag physics, a line-for-line port of arcade 1.0.3
 * (arcade/cabinet-snapshots/1.0.3/resources-app/dist/site/cornhole/index.html lines 193-514
 * and cornhole/spin.mjs). Doubles throughout, fixed 240 Hz steps, same loop order. */
#ifndef BYC_PHYSICS_H
#define BYC_PHYSICS_H

#include <stdbool.h>
#include "cornhole_constants.h"

#define BYC_PI 3.141592653589793

typedef struct { double x, y, z; } Vec3;

typedef enum { SIDE_SLICK = 0, SIDE_STICKY = 1 } BagSide;
typedef enum { ARC_SLIDER = 0, ARC_NORMAL = 1, ARC_KOBE = 2 } BagArc;
typedef enum {
    BAG_FLY, BAG_SLIDE, BAG_BOARD, BAG_GROUND, BAG_GRASS, BAG_HOLE, BAG_IN, BAG_REMOVED
} BagState;
typedef enum { EV_NONE, EV_HOLE, EV_LAND, EV_GROUND, EV_OFF, EV_REST, EV_REMOVED } StepEvent;

typedef struct {
    int team;
    BagSide side;
    double spin, tumble, english, curl, t, rest_t, hy, land_keep;
    BagArc arc;
    BagState state;
    Vec3 p, v, lp, lv, prev;
    bool touched, dead, swish, off_board;
    /* render-only state (never read by physics) */
    double yaw, release_yaw;
    bool has_yaw;
} Bag;

typedef struct { double x, z, mph, angle; } Wind;

/* Board frame, rebuilt from the same position/rotation as the Three.js deck group. */
typedef struct {
    double c, s;          /* cos/sin TH */
    Vec3 t;               /* deck origin in world */
    Vec3 hole_local, hole_world, g_local;
    double g_n, g_t;
} BoardFrame;
extern BoardFrame byc_board;

void physics_init(void);
Vec3 board_to_local(Vec3 world);
Vec3 board_to_world(Vec3 local);
Vec3 board_rot_inv(Vec3 v);   /* applyQuaternion(boardQi) */
Vec3 board_rot(Vec3 v);       /* applyQuaternion(boardQ) */
double base_yaw(double rx);
bool in_footprint(Vec3 p);
double hole_dist(Vec3 lp);
double arc_degrees(BagArc arc);

void bag_new(Bag *b, int team, BagSide side);
/* 1.0.3 launch(): yaw = baseYaw(rx) + aim + drift*0.04 (aims the line at the hole). */
void bag_launch(Bag *b, double rx, double power, double aim, double drift, BagArc arc, double spin);
/* Same physics with an explicit base yaw. The Wii human throw uses baseYaw(RX[team]) (= 0, straight
 * down-range) so a straight swing travels straight from the stance (owner directive: no auto-aim). */
void bag_launch_yaw(Bag *b, double rx, double yaw_base, double power, double aim, double drift, BagArc arc, double spin);
StepEvent bag_step(Bag *b, const Wind *wind, double dt);
void bags_collide(Bag *list, int count);
bool bag_on_deck(const Bag *b);
bool bag_is_final(const Bag *b);  /* board/grass/in/removed: candidate-search stop states */
/* index.html frame loop: step eligible bags in order, then collide once. */
void physics_substep(Bag *list, int count, const Wind *wind);
bool bags_settled(const Bag *list, int count);

double landing_curl(double spin, BagSide side);
double slide_velocity(Vec3 *v, double curl, BagSide side, double mu, Vec3 g, double g_n, double dt);

/* simulate(): 0 miss, 1 board, 3 in. */
int simulate_shot(double rx, double power, BagSide side, double aim, double drift, BagArc arc);

typedef struct { double lo, hi, olo, ohi, mid; } WinWindow;
/* WIN[team + side + arc] */
typedef struct { WinWindow w[2][2][3]; } WinTable;
void win_compute(WinTable *table);
extern const WinTable byc_win;   /* baked at build time (build/gen/win_table.c) */
double physical_power(const WinTable *win, int team, BagSide side, BagArc arc, double p);

#endif
