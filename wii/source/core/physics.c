#include "physics.h"
#include <math.h>
#include <string.h>

BoardFrame byc_board;
static const double RX[2] = {0, 0};

static double clampd(double v, double a, double b) { return fmax(a, fmin(b, v)); }

void physics_init(void) {
    BoardFrame *f = &byc_board;
    f->c = cos(BYC_TH); f->s = sin(BYC_TH);
    /* outer at (0,0,BOARD_Z); deckG at (0,FRONT_H,0) rotated TH about X */
    f->t = (Vec3){0, BYC_FRONT_H, BYC_BOARD_Z};
    f->hole_local = (Vec3){0, 0, -(BYC_L - BYC_HOLE_FROM_TOP)};
    f->hole_world = board_to_world(f->hole_local);
    f->g_local = board_rot_inv((Vec3){0, -BYC_G, 0});
    f->g_n = -f->g_local.y;
    f->g_t = hypot(f->g_local.x, f->g_local.z);
}
Vec3 board_rot(Vec3 v) {
    const BoardFrame *f = &byc_board;
    return (Vec3){v.x, f->c * v.y - f->s * v.z, f->s * v.y + f->c * v.z};
}
Vec3 board_rot_inv(Vec3 v) {
    const BoardFrame *f = &byc_board;
    return (Vec3){v.x, f->c * v.y + f->s * v.z, -f->s * v.y + f->c * v.z};
}
Vec3 board_to_world(Vec3 l) {
    Vec3 r = board_rot(l);
    return (Vec3){r.x + byc_board.t.x, r.y + byc_board.t.y, r.z + byc_board.t.z};
}
Vec3 board_to_local(Vec3 w) {
    return board_rot_inv((Vec3){w.x - byc_board.t.x, w.y - byc_board.t.y, w.z - byc_board.t.z});
}
double base_yaw(double rx) {
    return atan2(byc_board.hole_world.x - rx, -(byc_board.hole_world.z - BYC_REL_Z));
}
bool in_footprint(Vec3 p) {
    return fabs(p.x) < BYC_W / 2 + 0.02 && p.z < BYC_BOARD_Z + 0.02 && p.z > BYC_BOARD_Z - BYC_L * cos(BYC_TH);
}
double hole_dist(Vec3 lp) {
    return hypot(lp.x - byc_board.hole_local.x, lp.z - byc_board.hole_local.z);
}
double arc_degrees(BagArc arc) {
    return arc == ARC_SLIDER ? BYC_ARC_PUSH_DEG : arc == ARC_KOBE ? BYC_ARC_KOBE_DEG : BYC_ARC_NORMAL_DEG;
}

void bag_new(Bag *b, int team, BagSide side) {
    memset(b, 0, sizeof(*b));
    b->team = team; b->side = side; b->tumble = 1; b->arc = ARC_NORMAL; b->state = BAG_FLY;
}

static double surf_mu(BagSide s) { return s == SIDE_STICKY ? BYC_STICKY_MU : BYC_SLICK_MU; }
static double surf_ms(BagSide s) { return s == SIDE_STICKY ? BYC_STICKY_MS : BYC_SLICK_MS; }
static double surf_keep(BagSide s) { return s == SIDE_STICKY ? BYC_STICKY_KEEP : BYC_SLICK_KEEP; }

/* spin.mjs */
double landing_curl(double spin, BagSide side) {
    double accel = side == SIDE_STICKY ? BYC_CURL_STICKY_ACCEL : BYC_CURL_SLICK_ACCEL;
    return spin != 0 && !isnan(spin) ? -fmax(-1, fmin(1, spin)) * accel : 0;
}
double slide_velocity(Vec3 *v, double curl, BagSide side, double mu, Vec3 g, double g_n, double dt) {
    double decay = side == SIDE_STICKY ? BYC_CURL_STICKY_DECAY : BYC_CURL_SLICK_DECAY;
    v->x += (g.x + curl) * dt;
    v->z += g.z * dt;
    double speed = hypot(v->x, v->z), dec = mu * g_n * dt;
    if (speed <= dec) { v->x = 0; v->z = 0; }
    else { v->x -= v->x / speed * dec; v->z -= v->z / speed * dec; }
    return curl * exp(-decay * dt);
}

void bag_launch_yaw(Bag *b, double rx, double yaw_base, double power, double aim, double drift, BagArc arc, double spin) {
    double pwr = clampd(power, 0, 1);
    double speed = pwr < 0.4 ? BYC_V_MIN + pwr * 6 : 8 + (pwr - 0.4) * 3.2;
    double elev = arc_degrees(arc) * BYC_PI / 180;
    double velocity = speed * sqrt(sin(2 * BYC_ELEV) / sin(2 * elev));
    double y = yaw_base + aim + drift * 0.04;
    b->p = (Vec3){rx, BYC_REL_Y, BYC_REL_Z};
    b->v = (Vec3){sin(y) * cos(elev) * velocity, sin(elev) * velocity, -cos(y) * cos(elev) * velocity};
    b->arc = arc; b->english = spin; b->spin = spin * BYC_AIR_SPIN_RATE; b->curl = BYC_FLIGHT_CURL; b->state = BAG_FLY;
    b->prev = board_to_local(b->p);
}
void bag_launch(Bag *b, double rx, double power, double aim, double drift, BagArc arc, double spin) {
    bag_launch_yaw(b, rx, base_yaw(rx), power, aim, drift, arc, spin);
}

bool bag_on_deck(const Bag *b) { return b->state == BAG_SLIDE || b->state == BAG_BOARD; }
bool bag_is_final(const Bag *b) {
    return b->state == BAG_BOARD || b->state == BAG_GRASS || b->state == BAG_IN || b->state == BAG_REMOVED;
}

StepEvent bag_step(Bag *b, const Wind *wind, double dt) {
    const BoardFrame *f = &byc_board;
    b->t += dt;
    if (b->state == BAG_FLY) {
        b->v.x += wind->x * dt; b->v.z += wind->z * dt; b->v.y -= BYC_G * dt;
        b->p.x += b->v.x * dt; b->p.y += b->v.y * dt; b->p.z += b->v.z * dt;
        b->lp = board_to_local(b->p);
        Vec3 lp = b->lp;
        bool in_x = fabs(lp.x) <= BYC_W / 2, in_z = lp.z <= 0 && lp.z >= -BYC_L;
        if (in_x && in_z && lp.y <= BYC_BAG_H && lp.y > -0.14 && !b->off_board) {
            if (b->prev.y > BYC_BAG_H - 0.005) {
                b->lv = board_rot_inv(b->v);
                b->touched = true;
                if (hole_dist(lp) < BYC_HOLE_R * BYC_SWISH_R_FRAC) { b->state = BAG_HOLE; b->swish = true; b->hy = BYC_BAG_H; return EV_HOLE; }
                double k = surf_keep(b->side) * (b->arc == ARC_KOBE ? BYC_LAND_KEEP_KOBE : b->arc == ARC_SLIDER ? BYC_LAND_KEEP_PUSH : 1);
                b->lv.y = 0; b->lv.x *= k; b->lv.y *= k; b->lv.z *= k;
                b->lp.y = BYC_BAG_H; b->state = BAG_SLIDE; b->curl = landing_curl(b->english, b->side);
                return EV_LAND;
            } else if (b->prev.z > 0) { b->touched = true; b->v.z = fabs(b->v.z) * 0.2; b->v.x *= 0.4; b->p.z = BYC_BOARD_Z + 0.03; }
            else { b->touched = true; b->v.x *= -0.3; }
        }
        if (b->p.y <= BYC_BAG_H) {
            if (!b->touched) b->dead = true;
            b->p.y = BYC_BAG_H; b->v.y = 0; b->v.x *= 0.25; b->v.z *= 0.25; b->state = BAG_GROUND; return EV_GROUND;
        }
        b->prev = b->lp;
    } else if (b->state == BAG_SLIDE) {
        b->lp.y += (BYC_BAG_H - b->lp.y) * fmin(1, dt * 12);
        b->curl = slide_velocity(&b->lv, b->curl, b->side, surf_mu(b->side), f->g_local, f->g_n, dt);
        b->lp.x += b->lv.x * dt; b->lp.z += b->lv.z * dt;
        if (hole_dist(b->lp) < BYC_HOLE_R - BYC_SLIDE_IN_MARGIN) { b->state = BAG_HOLE; b->hy = BYC_BAG_H; return EV_HOLE; }
        if (fabs(b->lp.x) > BYC_W / 2 + 0.01 || b->lp.z > 0.01 || b->lp.z < -BYC_L - 0.01) {
            b->p = board_to_world(b->lp); b->v = board_rot(b->lv); b->state = BAG_FLY; b->off_board = true; return EV_OFF;
        }
        if (hypot(b->lv.x, b->lv.z) < 0.02 && f->g_t < surf_ms(b->side) * f->g_n) {
            b->lv = (Vec3){0, 0, 0}; b->state = BAG_BOARD; b->p = board_to_world(b->lp); return EV_REST;
        }
        b->p = board_to_world(b->lp);
    } else if (b->state == BAG_GROUND) {
        double sp = hypot(b->v.x, b->v.z), dec = BYC_MU_GROUND * BYC_G * dt;
        if (sp <= dec) { b->v = (Vec3){0, 0, 0}; b->state = BAG_GRASS; return EV_REST; }
        b->v.x -= b->v.x / sp * dec; b->v.z -= b->v.z / sp * dec;
        b->p.x += b->v.x * dt; b->p.z += b->v.z * dt;
        if (in_footprint(b->p)) { b->p.x -= b->v.x * dt; b->p.z -= b->v.z * dt; b->v = (Vec3){0, 0, 0}; }
    } else if (b->state == BAG_HOLE) {
        b->hy -= 0.7 * dt;
        b->lp = (Vec3){f->hole_local.x + (b->lp.x - f->hole_local.x) * 0.9, b->hy,
                       f->hole_local.z + (b->lp.z - f->hole_local.z) * 0.9};
        b->p = board_to_world(b->lp);
        if (b->hy < -0.09) { b->state = BAG_IN; return EV_REST; }
    } else if (b->state == BAG_GRASS && b->dead) {
        b->rest_t += dt; if (b->rest_t > 0.7) { b->state = BAG_REMOVED; return EV_REMOVED; }
    }
    return EV_NONE;
}

void bags_collide(Bag *list, int n) {
    for (int i = 0; i < n; i++) {
        Bag *a = &list[i];
        if (a->state == BAG_FLY && !a->off_board) {
            a->lp = board_to_local(a->p);
            for (int k = 0; k < n; k++) {
                Bag *b = &list[k];
                if (bag_on_deck(b) && hypot(a->lp.x - b->lp.x, a->lp.z - b->lp.z) < BYC_BAG_R * 1.8 &&
                    a->lp.y < b->lp.y + BYC_BAG_H * 2 && a->lp.y > b->lp.y && a->v.y < 0) {
                    a->lv = board_rot_inv(a->v); a->lv.y = 0;
                    double m = a->arc == ARC_KOBE ? 0.12 : 0.55;
                    a->lv.x *= m; a->lv.y *= m; a->lv.z *= m; a->lp.y = BYC_BAG_H * 3;
                    a->state = BAG_SLIDE; a->curl = landing_curl(a->english, a->side); a->touched = true;
                    a->p = board_to_world(a->lp);
                    break;
                }
            }
        }
        if (!bag_on_deck(a)) continue;
        for (int j = i + 1; j < n; j++) {
            Bag *b = &list[j];
            if (!bag_on_deck(b)) continue;
            double dx = b->lp.x - a->lp.x, dz = b->lp.z - a->lp.z, d = hypot(dx, dz);
            if (d >= 2 * BYC_BAG_R) continue;
            double nx = d > 1e-6 ? dx / d : 1, nz = d > 1e-6 ? dz / d : 0, ov = 2 * BYC_BAG_R - d;
            bool am = a->state == BAG_SLIDE;
            const double wa = 0.5;
            a->lp.x -= nx * ov * wa; a->lp.z -= nz * ov * wa; b->lp.x += nx * ov * (1 - wa); b->lp.z += nz * ov * (1 - wa);
            double vr = (b->lv.x - a->lv.x) * nx + (b->lv.z - a->lv.z) * nz;
            if (vr < 0) {
                double jn = -(1 + BYC_BAG_E) * vr / 2;
                a->lv.x -= jn * nx; a->lv.z -= jn * nz; b->lv.x += jn * nx; b->lv.z += jn * nz;
            }
            if (fabs(vr) > 0.8) { Bag *r = am ? a : b; r->lp.y = fmax(r->lp.y, BYC_BAG_H * 2.8); }
            a->p = board_to_world(a->lp); b->p = board_to_world(b->lp);
            Bag *pair[2] = {a, b};
            for (int c = 0; c < 2; c++)
                if (pair[c]->state == BAG_BOARD && (hypot(pair[c]->lv.x, pair[c]->lv.z) > 0.03 || ov > 0.004))
                    pair[c]->state = BAG_SLIDE;
        }
    }
}

void physics_substep(Bag *list, int n, const Wind *wind) {
    for (int i = 0; i < n; i++) {
        Bag *b = &list[i];
        if (b->state != BAG_IN && b->state != BAG_REMOVED && !(b->state == BAG_GRASS && !b->dead) && b->state != BAG_BOARD)
            bag_step(b, wind, BYC_PHYSICS_DT);
    }
    bags_collide(list, n);
}

bool bags_settled(const Bag *list, int n) {
    for (int i = 0; i < n; i++) {
        const Bag *b = &list[i];
        if (!bag_is_final(b) || (b->state == BAG_GRASS && b->dead)) return false;
    }
    return true;
}

int simulate_shot(double rx, double power, BagSide side, double aim, double drift, BagArc arc) {
    static const Wind calm = {0, 0, 0, 0};
    Bag b; bag_new(&b, 0, side); bag_launch(&b, rx, power, aim, drift, arc, 0);
    for (int i = 0; i < 240 * 8; i++) { bag_step(&b, &calm, 1.0 / 240); if (bag_is_final(&b)) break; }
    return b.state == BAG_IN ? 3 : b.state == BAG_BOARD ? 1 : 0;
}

void win_compute(WinTable *table) {
    for (int t = 0; t < 2; t++) for (int s = 0; s < 2; s++) for (int a = 0; a < 3; a++) {
        double lo = 1, hi = 0, olo = 1, ohi = 0;
        for (double p = 0; p <= 1; p += 0.0025) {
            int r = simulate_shot(RX[t], p, (BagSide)s, 0, 0, (BagArc)a);
            if (r == 3) { lo = fmin(lo, p); hi = fmax(hi, p); }
            if (r >= 1) { olo = fmin(olo, p); ohi = fmax(ohi, p); }
        }
        table->w[t][s][a] = (WinWindow){lo, hi, olo, ohi, hi >= lo ? (lo + hi) / 2 : (olo + ohi) / 2};
    }
}

/* index.html calibratedWindow()/physicalPower() with sweetShift = 0 (always 0 in 1.0.3). */
double physical_power(const WinTable *win, int team, BagSide side, BagArc arc, double p) {
    p = clampd(p, 0, 1);  /* throwPower() */
    if (arc != ARC_KOBE) return p;
    const WinWindow *base = &win->w[team][side][arc], *normal = &win->w[team][side][ARC_NORMAL];
    double width = (normal->hi - normal->lo) * .78;
    double lo = base->mid - width / 2, hi = base->mid + width / 2;
    if (p < lo) return p / lo * base->lo;
    if (p > hi) return base->hi + (p - hi) / (1 - hi) * (1 - base->hi);
    return base->lo + (p - lo) / (hi - lo) * (base->hi - base->lo);
}
