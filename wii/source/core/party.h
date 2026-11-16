/* Background "party" NPC sequences from the 1.0.3 cabinet (arcade/party.mjs): keg guest + friend
 * with the keg-stand gag, the scripted beer-pong match, and the grill cook who flips food and
 * tosses a burger to the dog. Platform-free port of the pure exports (pongCatalog, pongTiming,
 * buildPongMissPath, selectNext, buildPongScript, createPongMatch) and of the time-driven state
 * in createParty().update. Decoration only: nothing here touches bag collision or scoring.
 * Rendering lives in source/wii/party_draw.c. */
#ifndef BYC_PARTY_H
#define BYC_PARTY_H

#include <stdbool.h>
#include <stdint.h>
#include "rng.h"   /* Rng (stands in for Math.random) and Vec3 via physics.h */

/* ---------------- pure pong scripts ---------------- */
#define PONG_CATALOG_COUNT 5
#define PONG_MAX_TURNS 66          /* longest script is 33 + 32 shots */
#define PONG_CUPS 10
typedef struct { const char *id, *name, *shots[2]; } PongCatalogEntry;
extern const PongCatalogEntry pong_catalog[PONG_CATALOG_COUNT];

#define PONG_TURN_SECONDS 4.05
#define PONG_RELEASE_AT 1.18
#define PONG_BOUNCE_AT 1.83
#define PONG_SWAT_AT 2.12
#define PONG_IMPACT_AT 2.53
#define PONG_CELEBRATION_SECONDS 5.4

typedef enum { PONG_HIT, PONG_BOUNCE, PONG_MISS, PONG_SWAT } PongResult;
typedef enum { PONG_MISS_NONE, PONG_MISS_SHORT, PONG_MISS_LONG, PONG_MISS_RIM } PongMissType;
typedef struct { int side; PongResult result; bool scores; int round; int before[2]; } PongTurn;
typedef struct {
    int id;                         /* index into pong_catalog */
    PongTurn turns[PONG_MAX_TURNS];
    int turn_count, winner, hits[2];
    double match_seconds, loop_seconds;   /* matchSeconds, pongLoopSeconds */
} PongMatch;
typedef struct { PongMissType type; bool has_bounce; Vec3 bounce, target; } PongMissPath;

int pong_find(const char *id);                                   /* catalog index or -1 */
PongMissPath pong_miss_path(int side, int round, Vec3 cup);      /* buildPongMissPath */
int pong_select_next(int previous, double random);               /* selectNext; previous < 0 = null */
/* buildPongScript; swap is the `random() < .5` outcome and only applies to 'comeback'. false = no winner / bad id */
bool pong_build_script(int id, bool swap, PongMatch *out);
bool pong_create_match(int previous, Rng *rng, PongMatch *out);  /* createPongMatch (same draw order) */

/* ---------------- scene constants shared with the renderer ---------------- */
#define PARTY_PI 3.14159265358979323846
#define PARTY_KEG_X 3.7
#define PARTY_KEG_Z (-7.15)
#define PARTY_TABLE_X (-2.77)
#define PARTY_TABLE_Z (-7.65)
#define PARTY_TABLE_ROT_Y (PARTY_PI / 2)
#define PARTY_TABLE_TOP 0.64
#define PARTY_BALL_RADIUS 0.032
#define PARTY_COOK_X 3.0
#define PARTY_COOK_Z (-8.85)
#define PARTY_COOK_ROT_Y (-0.5)
#define PARTY_HOSE_SEGMENTS 18
#define PARTY_FOODS 4

/* Per-turn ball path in table-local coordinates (startMatch's pongTurns). */
typedef struct { Vec3 target, bounce, contact; bool has_bounce, has_contact; PongMissType miss_type; } PongShot;
/* A pong player's pose: group lift (cheer hop) and poseArm(shoulder, elbow, sweep). */
typedef struct { double y, shoulder, elbow, sweep; bool cheering; } PongPlayerPose;

typedef struct {
    double time;
    Rng rng;

    /* keg guest / friend / pump (update()) */
    double keg_t, keg_occasion;      /* time%20, time%65 */
    double stand;                    /* 0..1 keg-stand blend */
    bool keg_stand, fill, drink;
    double amount;                   /* approach amount 0..1 */
    Vec3 guest_pos;                  /* world */
    double guest_rot_x, guest_rot_y; /* group rotation (XYZ order, z = 0) */
    double guest_arm_x, guest_arm_z;
    bool guest_cup_visible, stream_visible, serving_visible;
    Vec3 friend_pos;                 /* world */
    double friend_rot_y, friend_arm_x, friend_arm_z, friend_arm_scale_y;
    double plunger_y;                /* keg-local */
    Vec3 tap_pos;                    /* keg-local */
    double squeeze_rot_x;
    Vec3 hose[PARTY_HOSE_SEGMENTS + 1];   /* keg-local curve.getPoint(i/18) */

    /* beer pong (updatePong()) */
    PongMatch match;
    PongShot shots[PONG_MAX_TURNS];
    Vec3 launch_points[2], swat_points[2];   /* table-local */
    double match_start;
    int matches_started;
    int turn;
    double phase;
    bool finished;
    int sunk[2], remaining[2];
    int winner;                      /* -1 = null */
    PongPlayerPose players[2];
    bool cup_visible[2][PONG_CUPS];  /* [rack side][index] */
    bool ball_visible;
    Vec3 ball_local, ball_world;     /* table-local / world; valid while ball_visible */

    /* grill cook (updateCook()) */
    double cook_cycle, flip;         /* time%36, (time%8)/1.3 */
    bool flipping;
    int food_index;                  /* floor(time/8) % 4 */
    double food_lift, food_rot_z;    /* applied to foods[food_index] while flipping */
    double cook_arm_x, cook_arm_z;
    bool toss;
    double toss_progress;
    Vec3 snack_from;                 /* world: spatula blade at the moment party.mjs samples it */
    double snack_rot_z;
    int catches, last_snack_cycle;
} Party;

void party_init(Party *p, uint32_t seed);
void party_update(Party *p, double dt);

/* Rack cup position (table-local, y = table top) for rack side 0 (x < 0) / 1 (x > 0). */
Vec3 party_rack_cup(int rack_side, int index);
/* handPosition(): table-local hand point for a pong player in the given pose. */
Vec3 party_pong_hand(int side, const PongPlayerPose *pose);
Vec3 party_table_to_world(Vec3 table_local);
/* Burger position for the current toss given the dog's mouth (world). */
Vec3 party_snack_position(const Party *p, Vec3 mouth_world);
/* dog.head.rotation.y the arcade sets during the toss: atan2(-toward.x, -toward.z) of the grill
 * point (2.9, .8, -8.6) in dog-local space. */
double party_dog_head_yaw(Vec3 dog_pos, double dog_rot_y);
/* Food base positions (world) and whether it is a steak. */
Vec3 party_food_base(int index, bool *steak);

#endif
