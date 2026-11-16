/* Host probe for the golden comparison (tests/physics_golden.mjs). Reads cases written by the
 * Node driver, runs them through the C core, writes results. No JSON parsing in C. */
#include <stdio.h>
#include <string.h>
#include "physics.h"
#include "rng.h"
#include "cpu.h"

static const char *state_name(BagState s) {
    static const char *n[] = {"fly", "slide", "board", "ground", "grass", "hole", "in", "removed"};
    return n[s];
}
static const char *event_name(StepEvent e) {
    static const char *n[] = {"", "hole", "land", "ground", "off", "rest", "removed"};
    return n[e];
}
static BagSide side_of(const char *s) { return strcmp(s, "sticky") == 0 ? SIDE_STICKY : SIDE_SLICK; }

int main(int argc, char **argv) {
    if (argc != 3) { fprintf(stderr, "usage: physics-probe in.txt out.txt\n"); return 2; }
    FILE *in = fopen(argv[1], "r"), *out = fopen(argv[2], "w");
    if (!in || !out) { perror("open"); return 2; }
    physics_init();
    char kind[16];
    while (fscanf(in, "%15s", kind) == 1) {
        if (!strcmp(kind, "win")) {
            WinTable t; win_compute(&t);
            for (int a = 0; a < 2; a++) for (int s = 0; s < 2; s++) for (int r = 0; r < 3; r++) {
                WinWindow w = t.w[a][s][r];
                fprintf(out, "win %d %d %d %.17g %.17g %.17g %.17g %.17g\n", a, s, r, w.lo, w.hi, w.olo, w.ohi, w.mid);
            }
        } else if (!strcmp(kind, "yaw")) {
            double x; if (fscanf(in, "%lf", &x) != 1) return 3;
            fprintf(out, "yaw %.17g\n", base_yaw(x));
        } else if (!strcmp(kind, "shot")) {
            char side[16]; int team, arc; double power, rx, aim, drift, spin, mph, angle;
            if (fscanf(in, "%d %15s %d %lf %lf %lf %lf %lf %lf %lf", &team, side, &arc, &power, &rx, &aim, &drift, &spin, &mph, &angle) != 10) return 3;
            Wind w = {0, 0, mph, angle};
            w.x = __builtin_sin(angle) * mph * .025; w.z = -__builtin_cos(angle) * mph * .025;
            Bag b; bag_new(&b, team, side_of(side)); bag_launch(&b, rx, power, aim, drift, (BagArc)arc, spin);
            fprintf(out, "shot");
            char trans[4096] = ""; size_t len = 0;
            for (int i = 0; i < 1920; i++) {
                StepEvent ev = bag_step(&b, &w, 1.0 / 240);
                if (ev != EV_NONE && len < sizeof(trans) - 64)
                    len += (size_t)snprintf(trans + len, sizeof(trans) - len, "%d:%s:%s;", i, event_name(ev), state_name(b.state));
                if (bag_is_final(&b)) break;
            }
            fprintf(out, " %s %.17g %.17g %.17g %.17g %.17g %.17g %d %d %d %d %.17g %s\n", state_name(b.state),
                    b.p.x, b.p.y, b.p.z, b.lp.x, b.lp.y, b.lp.z, b.dead, b.off_board, b.touched, b.swish, b.t, len ? trans : "-");
        } else if (!strcmp(kind, "scene")) {
            int nrest; if (fscanf(in, "%d", &nrest) != 1 || nrest > 6) return 3;
            Bag bags[8]; int n = 0;
            for (int k = 0; k < nrest; k++) {
                int team; double x, z; if (fscanf(in, "%d %lf %lf", &team, &x, &z) != 3) return 3;
                bag_new(&bags[n], team, SIDE_SLICK);
                bags[n].lp = (Vec3){x, BYC_BAG_H, z}; bags[n].p = board_to_world(bags[n].lp);
                bags[n].state = BAG_BOARD; bags[n].touched = true; n++;
            }
            char side[16]; int team, arc; double power, rx, aim, spin;
            if (fscanf(in, "%d %15s %d %lf %lf %lf %lf", &team, side, &arc, &power, &rx, &aim, &spin) != 7) return 3;
            bag_new(&bags[n], team, side_of(side)); bag_launch(&bags[n], rx, power, aim, 0, (BagArc)arc, spin); n++;
            Wind calm = {0, 0, 0, 0};
            int steps = 0;
            for (; steps < 240 * 10; steps++) { physics_substep(bags, n, &calm); if (bags_settled(bags, n)) break; }
            fprintf(out, "scene %d %d", steps, n);
            for (int k = 0; k < n; k++)
                fprintf(out, " %s %.17g %.17g %.17g %d", state_name(bags[k].state), bags[k].lp.x, bags[k].lp.y, bags[k].lp.z, bags[k].dead);
            fprintf(out, "\n");
        } else if (!strcmp(kind, "wind")) {
            int circuit; unsigned seed; if (fscanf(in, "%d %u", &circuit, &seed) != 2) return 3;
            Rng r; rng_seed(&r, seed); double prev = 0;
            fprintf(out, "wind");
            for (int round = 0; round < 4; round++) { Wind w = round_wind(false, circuit, prev, &r); fprintf(out, " %.17g %.17g", w.mph, w.angle); prev = w.x; }
            fprintf(out, "\n");
        } else if (!strcmp(kind, "plan")) {
            int level, nd; unsigned seed;
            if (fscanf(in, "%d %u %d", &level, &seed, &nd) != 3 || nd > 6) return 3;
            Bag deck[6];
            for (int k = 0; k < nd; k++) {
                int team; double x, z; if (fscanf(in, "%d %lf %lf", &team, &x, &z) != 3) return 3;
                bag_new(&deck[k], team, SIDE_SLICK); deck[k].lp = (Vec3){x, BYC_BAG_H, z};
                deck[k].p = board_to_world(deck[k].lp); deck[k].state = BAG_BOARD; deck[k].touched = true;
            }
            int scores[2]; double mph, angle;
            if (fscanf(in, "%d %d %lf %lf", &scores[0], &scores[1], &mph, &angle) != 4) return 3;
            Wind w = {__builtin_sin(angle) * mph * .025, -__builtin_cos(angle) * mph * .025, mph, angle};
            static CpuPlan plan; Rng r; rng_seed(&r, seed);
            cpu_plan_begin(&plan, &byc_roster[level], deck, nd, scores, &w);
            while (!cpu_plan_run(&plan, 5000, &r)) {}
            static const char *goals[] = {"hole", "push", "knock", "block"};
            fprintf(out, "plan %d %s %.17g %.17g %.17g %s %u\n", plan.result.arc, plan.result.side ? "sticky" : "slick",
                    plan.result.spin, plan.result.power, plan.result.aim, goals[plan.result.goal], r.draws);
        } else if (!strcmp(kind, "rng")) {
            unsigned seed; int count; if (fscanf(in, "%u %d", &seed, &count) != 2) return 3;
            Rng r; rng_seed(&r, seed); fprintf(out, "rng");
            for (int k = 0; k < count; k++) fprintf(out, " %.17g", rng_next(&r));
            fprintf(out, "\n");
        } else { fprintf(stderr, "bad case %s\n", kind); return 3; }
    }
    fclose(in); fclose(out);
    return 0;
}
