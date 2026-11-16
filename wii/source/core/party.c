/* Port of arcade/party.mjs (1.0.3 cabinet snapshot). See party.h. */
#include "party.h"
#include <math.h>
#include <string.h>

/* ---------------- small vector / matrix helpers (Three.js conventions) ---------------- */
typedef struct { double m[3][3]; } Mat3;

static Vec3 v3(double x, double y, double z) { Vec3 v = {x, y, z}; return v; }
static Vec3 v_add(Vec3 a, Vec3 b) { return v3(a.x + b.x, a.y + b.y, a.z + b.z); }
static Vec3 v_lerp(Vec3 a, Vec3 b, double t) { return v3(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t); }
/* Matrix4.makeRotationFromEuler, order 'XYZ' */
static Mat3 euler_xyz(double x, double y, double z) {
    double a = cos(x), b = sin(x), c = cos(y), d = sin(y), e = cos(z), f = sin(z);
    double ae = a * e, af = a * f, be = b * e, bf = b * f;
    Mat3 m = {{{c * e, -c * f, d}, {af + be * d, ae - bf * d, -b * c}, {bf - ae * d, be + af * d, a * c}}};
    return m;
}
static Vec3 m_apply(const Mat3 *m, Vec3 v) {
    return v3(m->m[0][0] * v.x + m->m[0][1] * v.y + m->m[0][2] * v.z,
              m->m[1][0] * v.x + m->m[1][1] * v.y + m->m[1][2] * v.z,
              m->m[2][0] * v.x + m->m[2][1] * v.y + m->m[2][2] * v.z);
}
static Vec3 rot_y(double angle, Vec3 v) {
    double c = cos(angle), s = sin(angle);
    return v3(c * v.x + s * v.z, v.y, -s * v.x + c * v.z);
}
/* THREE.MathUtils.smoothstep */
static double smoothstep(double x, double min, double max) {
    if (x <= min) return 0;
    if (x >= max) return 1;
    x = (x - min) / (max - min);
    return x * x * (3 - 2 * x);
}
static double lerp(double a, double b, double t) { return a + (b - a) * t; }
static double js_mod(double a, double b) { return fmod(a, b); }   /* time is never negative */

/* ---------------- pure pong scripts ---------------- */
const PongCatalogEntry pong_catalog[PONG_CATALOG_COUNT] = {
    {"red-blowout", "Red blowout", {"hmhbhhmhbhhh", "hmmsmhmhmms"}},
    {"blue-close", "Blue close", {"hmhhbhhmhhh", "hhhmbhhbhhh"}},
    {"comeback", "Comeback", {"mmmmhhbhhhhhhh", "hhhhhhmmsmsms"}},
    {"blue-solid", "Blue solid", {"hmhhmmhsmhhmmhm", "hhmhbhmhhmbhmhh"}},
    {"red-long", "Red long", {"hmsmhmsbhmsmhmsbhmsmhmsbhmsmhmsbh", "mhsmhmsmhmsbhmsmhmsbhmsmhmsmhmsm"}},
};
#define COMEBACK_ID 2

int pong_find(const char *id) {
    for (int i = 0; i < PONG_CATALOG_COUNT; i++) if (!strcmp(pong_catalog[i].id, id)) return i;
    return -1;
}

/* Six of eight rounds clip the next standing cup; the others land on the table. */
PongMissPath pong_miss_path(int side, int round, Vec3 cup) {
    PongMissPath path;
    double end = side == 0 ? 1 : -1;
    int kind = round % 8;
    memset(&path, 0, sizeof path);
    if (kind < 2) {
        path.type = kind == 0 ? PONG_MISS_SHORT : PONG_MISS_LONG;
        path.has_bounce = false;
        path.target = v3(end * (kind == 0 ? .34 : .99), .672, cup.z);
        return path;
    }
    double direction = round % 2 == 0 ? -1 : 1;
    bool ground = round % 3 == 0;
    path.type = PONG_MISS_RIM;
    path.has_bounce = true;
    path.bounce = v3(cup.x - end * .025, .795, cup.z + direction * .055);
    path.target = v3(cup.x - end * .12, ground ? .032 : .672, direction * (ground ? .50 : .34));
    return path;
}

int pong_select_next(int previous, double random) {
    int choices[PONG_CATALOG_COUNT], n = 0;
    for (int i = 0; i < PONG_CATALOG_COUNT; i++) if (i != previous) choices[n++] = i;
    int pick = (int)floor(random * n);
    if (pick < 0) pick = 0;
    if (pick >= n) pick = n - 1;
    return choices[pick];
}

bool pong_build_script(int id, bool swap, PongMatch *out) {
    memset(out, 0, sizeof *out);
    out->id = id; out->winner = -1;
    if (id < 0 || id >= PONG_CATALOG_COUNT) return false;
    const PongCatalogEntry *entry = &pong_catalog[id];
    swap = swap && id == COMEBACK_ID;   /* party.mjs only ever mirrors 'comeback' */
    int length[2] = {(int)strlen(entry->shots[0]), (int)strlen(entry->shots[1])};
    int rounds = length[0] > length[1] ? length[0] : length[1];
    int hits[2] = {0, 0};
    for (int round = 0; round < rounds; round++) {
        for (int source = 0; source < 2; source++) {
            if (round >= length[source]) continue;
            PongResult result;
            switch (entry->shots[source][round]) {
                case 'h': result = PONG_HIT; break;
                case 'b': result = PONG_BOUNCE; break;
                case 'm': result = PONG_MISS; break;
                case 's': result = PONG_SWAT; break;
                default: continue;
            }
            if (out->turn_count >= PONG_MAX_TURNS) return false;
            int side = swap ? 1 - source : source;
            bool scores = result == PONG_HIT || result == PONG_BOUNCE;
            PongTurn *turn = &out->turns[out->turn_count++];
            turn->side = side; turn->result = result; turn->scores = scores; turn->round = round;
            turn->before[0] = hits[0]; turn->before[1] = hits[1];
            if (scores && ++hits[side] == PONG_CUPS) {
                out->winner = side; out->hits[0] = hits[0]; out->hits[1] = hits[1];
                out->match_seconds = (out->turn_count - 1) * PONG_TURN_SECONDS + PONG_IMPACT_AT;
                out->loop_seconds = out->match_seconds + PONG_CELEBRATION_SECONDS;
                return true;
            }
        }
    }
    return false;   /* 'Pong script has no winner' */
}

bool pong_create_match(int previous, Rng *rng, PongMatch *out) {
    int id = pong_select_next(previous, rng_next(rng));
    bool swap = id == COMEBACK_ID && rng_next(rng) < .5;   /* the second draw only happens for 'comeback' */
    return pong_build_script(id, swap, out);
}

/* ---------------- scene geometry needed for the ball ---------------- */
Vec3 party_rack_cup(int rack_side, int index) {
    double end = rack_side == 0 ? -1 : 1;
    int i = 0;
    for (int row = 0; row < 4; row++) for (int slot = 0; slot <= row; slot++, i++)
        if (i == index) return v3(end * (.45 + row * .146), PARTY_TABLE_TOP, (slot - row / 2.0) * .146);
    return v3(0, PARTY_TABLE_TOP, 0);
}

Vec3 party_pong_hand(int side, const PongPlayerPose *pose) {
    /* pong-red: (-1.29, y, -.22) rot PI/2 handedness -1; pong-blue: (1.29, y, .12) rot -PI/2 handedness +1 */
    double handedness = side == 0 ? -1 : 1;
    Mat3 elbow = euler_xyz(pose->elbow, 0, 0), arm = euler_xyz(pose->shoulder, 0, pose->sweep);
    Vec3 p = m_apply(&elbow, v3(0, -.16, .02));       /* handPoint in the elbow group */
    p.y += -.15;                                      /* elbow group in the arm */
    p = m_apply(&arm, p);
    p = v_add(p, v3(.175 * handedness, .79, 0));      /* arm group in the player */
    p = rot_y(side == 0 ? PARTY_PI / 2 : -PARTY_PI / 2, p);
    return v_add(p, side == 0 ? v3(-1.29, pose->y, -.22) : v3(1.29, pose->y, .12));
}

Vec3 party_table_to_world(Vec3 t) {
    return v_add(rot_y(PARTY_TABLE_ROT_Y, t), v3(PARTY_TABLE_X, 0, PARTY_TABLE_Z));
}

static const double release_pose[2] = {-2.12, -.45};
static const double swat_pose[3] = {-1.62, -.38, .38};

static void start_match(Party *p, int previous) {
    PongMatch *m = &p->match;
    if (!pong_create_match(previous, &p->rng, m)) {
        /* Unreachable with the shipped catalog (every script has a winner; see party_test.c). */
        pong_build_script(0, false, m);
    }
    for (int i = 0; i < m->turn_count; i++) {
        const PongTurn *shot = &m->turns[i];
        PongShot *s = &p->shots[i];
        int side = shot->side;
        memset(s, 0, sizeof *s);
        Vec3 target = party_rack_cup(1 - side, shot->before[side]);
        target.y += .13;
        PongMissPath miss;
        memset(&miss, 0, sizeof miss);
        if (shot->result == PONG_MISS) { miss = pong_miss_path(side, shot->round, target); target = miss.target; }
        if (miss.has_bounce) { s->has_bounce = true; s->bounce = miss.bounce; }
        else if (shot->result == PONG_BOUNCE || shot->result == PONG_SWAT) {
            s->has_bounce = true;
            s->bounce = v3((side == 0 ? 1 : -1) * .43, PARTY_TABLE_TOP + PARTY_BALL_RADIUS, target.z * .45);
        }
        if (shot->result == PONG_SWAT) {
            s->has_contact = true;
            s->contact = p->swat_points[1 - side];
            target = v_add(s->contact, v3(side == 0 ? .25 : -.25, -.37, side == 0 ? .48 : -.48));
        }
        s->target = target;
        s->miss_type = miss.type;
    }
    p->turn = 0; p->winner = -1; p->remaining[0] = p->remaining[1] = PONG_CUPS;
    for (int side = 0; side < 2; side++) for (int i = 0; i < PONG_CUPS; i++) p->cup_visible[side][i] = true;
    p->matches_started++;
}

static Vec3 move_ball(Vec3 from, Vec3 to, double fraction, double arc) {
    Vec3 b = v_lerp(from, to, fraction);
    b.y += arc * 4 * fraction * (1 - fraction);
    return b;
}

static void update_pong(Party *p) {
    while (p->time - p->match_start >= p->match.loop_seconds) {
        p->match_start += p->match.loop_seconds;
        start_match(p, p->match.id);
    }
    const PongMatch *m = &p->match;
    double elapsed = p->time - p->match_start;
    bool finished = elapsed >= m->match_seconds;
    int turn = (int)floor(elapsed / PONG_TURN_SECONDS);
    if (turn > m->turn_count - 1) turn = m->turn_count - 1;
    if (turn < 0) turn = 0;
    const PongTurn *shot = &m->turns[turn];
    const PongShot *path = &p->shots[turn];
    double phase = elapsed - turn * PONG_TURN_SECONDS;
    int sunk[2] = {shot->before[0], shot->before[1]};
    if (phase >= PONG_IMPACT_AT && shot->scores) sunk[shot->side]++;
    p->turn = turn; p->phase = phase; p->finished = finished;
    p->sunk[0] = sunk[0]; p->sunk[1] = sunk[1];
    p->remaining[0] = PONG_CUPS - sunk[1]; p->remaining[1] = PONG_CUPS - sunk[0];
    p->winner = sunk[shot->side] == PONG_CUPS ? shot->side : -1;
    for (int side = 0; side < 2; side++) for (int i = 0; i < PONG_CUPS; i++) p->cup_visible[side][i] = i >= sunk[1 - side];
    for (int side = 0; side < 2; side++) {
        PongPlayerPose *pose = &p->players[side];
        bool cheering = p->winner == side;
        double cheer = cheering ? fabs(sin((phase - PONG_IMPACT_AT) * 5)) : 0;
        pose->cheering = cheering;
        pose->y = cheer * .13;
        pose->shoulder = -.2; pose->elbow = 0; pose->sweep = 0;
        if (cheering) {
            pose->shoulder = -2.8 + cheer * .28; pose->elbow = -.4;
        } else if (!finished && side == shot->side && phase < 1.58) {
            /* Raise the ball beside the face, extend the forearm, then follow through. */
            double windup = smoothstep(phase, .12, .85);
            double release = smoothstep(phase, .85, PONG_RELEASE_AT);
            double settle = smoothstep(phase, PONG_RELEASE_AT, 1.58);
            pose->shoulder = -.2 - 2.35 * windup + .43 * release + 1.92 * settle;
            pose->elbow = -1.25 * windup + .8 * release + .45 * settle;
        } else if (path->has_contact && side != shot->side) {
            double reach = phase <= PONG_SWAT_AT
                ? smoothstep(phase, PONG_BOUNCE_AT, PONG_SWAT_AT)
                : 1 - smoothstep(phase, PONG_SWAT_AT, PONG_SWAT_AT + .34);
            pose->shoulder = lerp(-.2, swat_pose[0], reach);
            pose->elbow = swat_pose[1] * reach;
            pose->sweep = (side == 0 ? -1 : 1) * swat_pose[2] * reach;
        }
    }
    p->ball_visible = !finished && phase < PONG_IMPACT_AT;
    if (!p->ball_visible) return;
    Vec3 launch = p->launch_points[shot->side], ball;
    if (phase < PONG_RELEASE_AT) ball = party_pong_hand(shot->side, &p->players[shot->side]);
    else if (!path->has_bounce) ball = move_ball(launch, path->target, (phase - PONG_RELEASE_AT) / (PONG_IMPACT_AT - PONG_RELEASE_AT), .28);
    else if (phase < PONG_BOUNCE_AT) ball = move_ball(launch, path->bounce, (phase - PONG_RELEASE_AT) / (PONG_BOUNCE_AT - PONG_RELEASE_AT), .14);
    else if (!path->has_contact) ball = move_ball(path->bounce, path->target, (phase - PONG_BOUNCE_AT) / (PONG_IMPACT_AT - PONG_BOUNCE_AT), .21);
    else if (phase < PONG_SWAT_AT) ball = move_ball(path->bounce, path->contact, (phase - PONG_BOUNCE_AT) / (PONG_SWAT_AT - PONG_BOUNCE_AT), .17);
    else ball = move_ball(path->contact, path->target, (phase - PONG_SWAT_AT) / (PONG_IMPACT_AT - PONG_SWAT_AT), .09);
    p->ball_local = ball;
    p->ball_world = party_table_to_world(ball);
}

/* ---------------- hose: THREE.CatmullRomCurve3 (centripetal, open) ---------------- */
static double dist_sq(Vec3 a, Vec3 b) { double x = a.x - b.x, y = a.y - b.y, z = a.z - b.z; return x * x + y * y + z * z; }
static double cubic_nonuniform(double x0, double x1, double x2, double x3, double dt0, double dt1, double dt2, double t) {
    double t1 = (x1 - x0) / dt0 - (x2 - x0) / (dt0 + dt1) + (x2 - x1) / dt1;
    double t2 = (x2 - x1) / dt1 - (x3 - x1) / (dt1 + dt2) + (x3 - x2) / dt2;
    t1 *= dt1; t2 *= dt1;
    double c0 = x1, c1 = t1, c2 = -3 * x1 + 3 * x2 - 2 * t1 - t2, c3 = 2 * x1 - 2 * x2 + t1 + t2;
    return c0 + c1 * t + c2 * t * t + c3 * t * t * t;
}
static Vec3 catmull_point(const Vec3 *points, int l, double t) {
    double pos = (l - 1) * t;
    int i = (int)floor(pos);
    double weight = pos - i;
    if (i >= l - 1) { i = l - 2; weight = 1; }
    Vec3 p1 = points[i], p2 = points[i + 1], p0, p3;
    if (i > 0) p0 = points[i - 1];
    else p0 = v3(2 * points[0].x - points[1].x, 2 * points[0].y - points[1].y, 2 * points[0].z - points[1].z);
    if (i + 2 < l) p3 = points[i + 2];
    else p3 = v3(2 * points[l - 1].x - points[l - 2].x, 2 * points[l - 1].y - points[l - 2].y, 2 * points[l - 1].z - points[l - 2].z);
    double dt0 = pow(dist_sq(p0, p1), .25), dt1 = pow(dist_sq(p1, p2), .25), dt2 = pow(dist_sq(p2, p3), .25);
    if (dt1 < 1e-4) dt1 = 1;
    if (dt0 < 1e-4) dt0 = dt1;
    if (dt2 < 1e-4) dt2 = dt1;
    return v3(cubic_nonuniform(p0.x, p1.x, p2.x, p3.x, dt0, dt1, dt2, weight),
              cubic_nonuniform(p0.y, p1.y, p2.y, p3.y, dt0, dt1, dt2, weight),
              cubic_nonuniform(p0.z, p1.z, p2.z, p3.z, dt0, dt1, dt2, weight));
}
static void shape_hose(Party *p) {
    Vec3 points[5] = {v3(.12, .98, .015), v3(.37, 1.02, .13), v3(.43, .55, .34), v3(.18, .48, .52),
                      v3(p->tap_pos.x, p->tap_pos.y, p->tap_pos.z - .07)};
    for (int i = 0; i <= PARTY_HOSE_SEGMENTS; i++) p->hose[i] = catmull_point(points, 5, (double)i / PARTY_HOSE_SEGMENTS);
}

/* ---------------- keg ---------------- */
#define KEG_X 5.4
#define KEG_Z (-.45)

static void update_keg(Party *p) {
    double time = p->time, t = js_mod(time, 20), occasion = js_mod(time, 65);
    double stand = 0;
    if (occasion >= 48 && occasion < 57) stand = fmin(1, fmin((occasion - 48) / 1.5, (57 - occasion) / 1.5));
    /* Approach, fill, take a drink, then wander back to a friend, always left of play. */
    bool fill = t >= 4 && t < 8, drink = t >= 9 && t < 13;
    double amount = t < 4 ? t / 4 : t < 13 ? 1 : t < 17 ? 1 - (t - 13) / 4 : 0;
    p->keg_t = t; p->keg_occasion = occasion; p->stand = stand; p->keg_stand = stand > 0;
    p->fill = fill; p->drink = drink; p->amount = amount;
    p->guest_pos = v3(-2.45 + KEG_X + .48 * amount, 0, -5.7 + KEG_Z - .38 * amount);
    p->guest_rot_y = fill ? -.45 : drink ? -.1 : .5;
    p->guest_pos.y = (t < 4 || (t >= 13 && t < 17)) ? sin(t * 8) * .012 : 0;
    p->guest_arm_x = fill ? -1.05 : drink ? -2.4 : -.15;
    p->guest_arm_z = fill ? -.65 : drink ? -.15 : 0;
    p->stream_visible = p->serving_visible = fill;
    p->guest_cup_visible = !fill;
    p->friend_rot_y = .8 + sin(time * .3) * .13;
    p->friend_arm_x = -.3 - fmax(0, sin(time * .45)) * .5;
    p->plunger_y = 1.27 + (fill ? .045 * sin(time * 8) : .025);
    p->tap_pos = v3(0, .79 + .09 * stand, .5 - .141 * stand);
    p->squeeze_rot_x = (fill || stand > 0) ? -.7 : -.35;
    shape_hose(p);
    /* One short gag per 65-second cycle; blend into/out of an inverted pose. */
    p->guest_rot_x = PARTY_PI * stand;
    if (stand > 0) {
        p->guest_pos = v_lerp(p->guest_pos, v3(PARTY_KEG_X, 1.85, PARTY_KEG_Z + .48), stand);
        p->guest_rot_y *= 1 - stand;
        p->friend_pos = v3(PARTY_KEG_X - .65 + .2 * stand, 0, PARTY_KEG_Z - .55 + .9 * stand);
        p->friend_rot_y *= 1 - stand;
        p->friend_arm_z = 2.45 * stand; p->friend_arm_scale_y = 1 + .6 * stand;
        p->guest_cup_visible = p->serving_visible = p->stream_visible = false;
    } else {
        p->friend_pos = v3(PARTY_KEG_X - .65, 0, PARTY_KEG_Z - .55);
        p->friend_arm_z = 0; p->friend_arm_scale_y = 1;
    }
}

/* ---------------- cook ---------------- */
Vec3 party_food_base(int index, bool *steak) {
    static const double base[PARTY_FOODS][2] = {{2.58, -8.25}, {2.78, -8.45}, {2.9, -8.25}, {2.72, -8.25}};
    if (index < 0) index = 0;
    if (index >= PARTY_FOODS) index = PARTY_FOODS - 1;
    if (steak) *steak = index < 2;
    return v3(base[index][0], .75, base[index][1]);
}

static Vec3 spatula_blade_world(double arm_x, double arm_z) {
    /* cook.g (3, 0, -8.85) rot.y -.5 -> arm (.175, .79, 0) rotation (x, 0, z) -> spatula (0, -.3, .03) -> blade (0, -.24, 0) */
    Mat3 arm = euler_xyz(arm_x, 0, arm_z);
    Vec3 q = v_add(m_apply(&arm, v3(0, -.54, .03)), v3(.175, .79, 0));
    return v_add(rot_y(PARTY_COOK_ROT_Y, q), v3(PARTY_COOK_X, 0, PARTY_COOK_Z));
}

static void update_cook(Party *p) {
    double time = p->time, cycle = js_mod(time, 36), flip = js_mod(time, 8) / 1.3;
    bool flipping = flip < 1;
    p->cook_cycle = cycle; p->flip = flip; p->flipping = flipping;
    p->cook_arm_x = -.8 - (flipping ? sin(flip * PARTY_PI) * 1.4 : 0);
    p->cook_arm_z = -.5;
    p->food_index = (int)floor(time / 8) % PARTY_FOODS;
    p->food_lift = flipping ? sin(flip * PARTY_PI) * .25 : 0;
    p->food_rot_z = flipping ? flip * PARTY_PI * 2 : 0;
    bool toss = cycle >= 25 && cycle < 26.4;
    double progress = (cycle - 25) / 1.4;
    p->toss = toss;
    p->toss_progress = toss ? progress : 0;
    p->snack_rot_z = 0;
    if (toss) {
        /* party.mjs samples the spatula before it forces the arm to -1.9 for this frame. */
        p->snack_from = spatula_blade_world(p->cook_arm_x, p->cook_arm_z);
        p->snack_rot_z = progress * PARTY_PI * 3;
        p->cook_arm_x = -1.9;
    }
    int snack_cycle = (int)floor(time / 36);
    if (cycle >= 26.4 && snack_cycle != p->last_snack_cycle) { p->last_snack_cycle = snack_cycle; p->catches++; }
}

Vec3 party_snack_position(const Party *p, Vec3 mouth) {
    Vec3 s = v_lerp(p->snack_from, mouth, p->toss_progress);
    s.y += sin(p->toss_progress * PARTY_PI) * .75;
    return s;
}

double party_dog_head_yaw(Vec3 dog_pos, double dog_rot_y) {
    /* dog.g.worldToLocal((2.9, .8, -8.6)): the uniform scale does not change the angle. */
    Vec3 toward = rot_y(-dog_rot_y, v3(2.9 - dog_pos.x, .8 - dog_pos.y, -8.6 - dog_pos.z));
    return atan2(-toward.x, -toward.z);
}

/* ---------------- lifecycle ---------------- */
void party_init(Party *p, uint32_t seed) {
    memset(p, 0, sizeof *p);
    rng_seed(&p->rng, seed);
    p->winner = -1;
    p->last_snack_cycle = -1;
    for (int side = 0; side < 2; side++) {
        PongPlayerPose release = {0, release_pose[0], release_pose[1], 0, false};
        PongPlayerPose swat = {0, swat_pose[0], swat_pose[1], (side == 0 ? -1 : 1) * swat_pose[2], false};
        p->launch_points[side] = party_pong_hand(side, &release);
        p->swat_points[side] = party_pong_hand(side, &swat);
    }
    start_match(p, -1);
    party_update(p, 0);   /* populate the poses for time 0 (no RNG draws, no counters change) */
}

void party_update(Party *p, double dt) {
    p->time += dt;
    update_keg(p);
    update_pong(p);
    update_cook(p);
}
