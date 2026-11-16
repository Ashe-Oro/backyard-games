/* Host tests for the background party port (source/core/party.c <- arcade/party.mjs 1.0.3):
 * pong catalog scripts, selectNext, miss paths, keg / keg-stand timeline, cook flip + burger toss,
 * pong loop bookkeeping, ball visibility, and a 30-minute determinism / finiteness soak.
 *
 * Values marked "JS:" were produced by running the cabinet snapshot
 * arcade/cabinet-snapshots/1.0.3/resources-app/dist/site/arcade/party.mjs under node (three from
 * arcade/node_modules): buildPongScript / createPongMatch directly, and createParty(scene,
 * {random: mulberry32(1234)}) stepped with update(1/60, dog) where dog.g is at (2.3, 0, -8.7),
 * rotation.y 2.6, scale .85 and dog.head at (0, .55, -.15). */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "party.h"

static int failures, checks;
#define CHECK(cond, ...) do { checks++; if (!(cond)) { failures++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)
#define NEAR(a, b, eps) (fabs((double)(a) - (double)(b)) <= (eps))
#define CHECK_NEAR(a, b, eps) CHECK(NEAR(a, b, eps), "%s = %.9f, expected %.9f", #a, (double)(a), (double)(b))

static Party party;
static void run_frames(Party *p, int frames) { for (int i = 0; i < frames; i++) party_update(p, 1.0 / 60); }
/* A fresh party advanced in one step to an exact time (the state is a pure function of time
 * apart from match restarts, so this is used only inside the first match). */
static void at(double t) { party_init(&party, 1234); party_update(&party, t); }

static void test_catalog(void) {
    /* JS: turns, winner, hits and matchSeconds per catalog id (swap = false / true). */
    static const struct { const char *id; int turns, winner[2], hits[2][2]; double seconds; } expect[PONG_CATALOG_COUNT] = {
        {"red-blowout", 23, {0, 0}, {{10, 3}, {10, 3}}, 91.63},
        {"blue-close", 22, {1, 1}, {{9, 10}, {9, 10}}, 87.58},
        {"comeback", 27, {0, 1}, {{10, 6}, {6, 10}}, 107.83},
        {"blue-solid", 28, {1, 1}, {{7, 10}, {7, 10}}, 111.88},
        {"red-long", 49, {0, 0}, {{10, 8}, {10, 8}}, 196.93},
    };
    for (int id = 0; id < PONG_CATALOG_COUNT; id++) {
        CHECK(!strcmp(pong_catalog[id].id, expect[id].id), "catalog[%d] id %s", id, pong_catalog[id].id);
        CHECK(pong_find(expect[id].id) == id, "pong_find(%s)", expect[id].id);
        for (int swap = 0; swap < 2; swap++) {
            PongMatch m;
            bool ok = pong_build_script(id, swap != 0, &m);
            CHECK(ok, "%s has a winner", expect[id].id);
            CHECK(m.turn_count == expect[id].turns, "%s turns %d", expect[id].id, m.turn_count);
            CHECK(m.winner == expect[id].winner[swap], "%s swap %d winner %d", expect[id].id, swap, m.winner);
            CHECK(m.hits[0] == expect[id].hits[swap][0] && m.hits[1] == expect[id].hits[swap][1], "%s swap %d hits %d,%d", expect[id].id, swap, m.hits[0], m.hits[1]);
            CHECK(m.hits[m.winner] == PONG_CUPS, "%s winner has 10 hits", expect[id].id);
            CHECK_NEAR(m.match_seconds, expect[id].seconds, 1e-9);
            CHECK_NEAR(m.match_seconds, (m.turn_count - 1) * 4.05 + 2.53, 1e-12);
            CHECK_NEAR(m.loop_seconds, m.match_seconds + 5.4, 1e-12);
            /* turn / before bookkeeping: replay the hits and compare with every turn's snapshot */
            int hits[2] = {0, 0}, last_round = 0;
            for (int i = 0; i < m.turn_count; i++) {
                const PongTurn *t = &m.turns[i];
                CHECK(t->side == 0 || t->side == 1, "%s turn %d side", expect[id].id, i);
                CHECK(t->before[0] == hits[0] && t->before[1] == hits[1], "%s turn %d before %d,%d", expect[id].id, i, t->before[0], t->before[1]);
                CHECK(t->scores == (t->result == PONG_HIT || t->result == PONG_BOUNCE), "%s turn %d scores", expect[id].id, i);
                CHECK(t->round >= last_round, "%s turn %d round order", expect[id].id, i);
                CHECK(t->before[t->side] < PONG_CUPS, "%s turn %d aims at a standing cup", expect[id].id, i);
                last_round = t->round;
                if (t->scores) hits[t->side]++;
                if (i < m.turn_count - 1) CHECK(hits[0] < PONG_CUPS && hits[1] < PONG_CUPS, "%s ends on the tenth hit", expect[id].id);
            }
            CHECK(m.turns[m.turn_count - 1].scores && m.turns[m.turn_count - 1].side == m.winner, "%s last turn wins", expect[id].id);
        }
    }
    /* Only 'comeback' mirrors the sides when swapped; first shooter flips with it. */
    PongMatch a, b;
    pong_build_script(pong_find("comeback"), false, &a);
    pong_build_script(pong_find("comeback"), true, &b);
    CHECK(a.turns[0].side == 0 && b.turns[0].side == 1, "comeback swap flips the first shooter");
    for (int i = 0; i < a.turn_count; i++) CHECK(a.turns[i].side == 1 - b.turns[i].side && a.turns[i].result == b.turns[i].result, "comeback swap turn %d", i);
    CHECK(!pong_build_script(-1, false, &a) && !pong_build_script(PONG_CATALOG_COUNT, false, &a), "unknown ids are rejected");
    CHECK(pong_find("nope") == -1, "pong_find unknown");
}

static void test_select_next(void) {
    for (int previous = -1; previous < PONG_CATALOG_COUNT; previous++) {
        bool seen[PONG_CATALOG_COUNT] = {false};
        for (int k = 0; k < 1000; k++) {
            int id = pong_select_next(previous, k / 1000.0);
            CHECK(id >= 0 && id < PONG_CATALOG_COUNT && id != previous, "selectNext(%d) -> %d", previous, id);
            if (id >= 0 && id < PONG_CATALOG_COUNT) seen[id] = true;
        }
        for (int id = 0; id < PONG_CATALOG_COUNT; id++) CHECK(seen[id] == (id != previous), "selectNext(%d) reaches %d", previous, id);
    }
    CHECK(pong_select_next(-1, 0) == 0 && pong_select_next(-1, .999) == 4, "null previous picks from all five");
    CHECK(pong_select_next(0, 0) == 1 && pong_select_next(4, .999) == 3, "previous is filtered out");
    /* JS: createPongMatch chained with mulberry32(1234): catalog index, winner, turns. */
    static const int ids[12] = {0, 3, 4, 3, 0, 1, 0, 4, 1, 0, 3, 1}, winners[12] = {0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1};
    static const int turns[12] = {23, 28, 49, 28, 23, 22, 23, 49, 22, 23, 28, 22};
    Rng rng; rng_seed(&rng, 1234);
    int previous = -1;
    for (int i = 0; i < 12; i++) {
        PongMatch m;
        CHECK(pong_create_match(previous, &rng, &m), "match %d built", i);
        CHECK(m.id == ids[i] && m.winner == winners[i] && m.turn_count == turns[i], "seed 1234 match %d: id %d winner %d turns %d", i, m.id, m.winner, m.turn_count);
        previous = m.id;
    }
    /* Draw order: one draw, plus a second only for 'comeback'. Both swap outcomes are reachable. */
    bool swapped = false, straight = false;
    for (uint32_t seed = 1; seed < 400; seed++) {
        PongMatch m; rng_seed(&rng, seed);
        pong_create_match(-1, &rng, &m);
        CHECK(rng.draws == (m.id == pong_find("comeback") ? 2u : 1u), "seed %u draws %u", (unsigned)seed, (unsigned)rng.draws);
        if (m.id == pong_find("comeback")) { if (m.winner == 1) swapped = true; else straight = true; }
    }
    CHECK(swapped && straight, "comeback is seen both swapped and unswapped");
}

static void test_miss_paths(void) {
    Vec3 cup = {.596, .77, -.073};
    for (int side = 0; side < 2; side++) {
        double end = side == 0 ? 1 : -1;
        for (int round = 0; round < 24; round++) {
            PongMissPath m = pong_miss_path(side, round, cup);
            int kind = round % 8;
            if (kind == 0) {
                CHECK(m.type == PONG_MISS_SHORT && !m.has_bounce, "round %d short", round);
                CHECK_NEAR(m.target.x, end * .34, 1e-12); CHECK_NEAR(m.target.y, .672, 1e-12); CHECK_NEAR(m.target.z, cup.z, 1e-12);
            } else if (kind == 1) {
                CHECK(m.type == PONG_MISS_LONG && !m.has_bounce, "round %d long", round);
                CHECK_NEAR(m.target.x, end * .99, 1e-12); CHECK_NEAR(m.target.z, cup.z, 1e-12);
            } else {
                double direction = round % 2 == 0 ? -1 : 1;
                bool ground = round % 3 == 0;
                CHECK(m.type == PONG_MISS_RIM && m.has_bounce, "round %d rim", round);
                CHECK_NEAR(m.bounce.x, cup.x - end * .025, 1e-12); CHECK_NEAR(m.bounce.y, .795, 1e-12); CHECK_NEAR(m.bounce.z, cup.z + direction * .055, 1e-12);
                CHECK_NEAR(m.target.x, cup.x - end * .12, 1e-12);
                CHECK_NEAR(m.target.y, ground ? .032 : .672, 1e-12);
                CHECK_NEAR(m.target.z, direction * (ground ? .50 : .34), 1e-12);
            }
        }
    }
}

static void test_geometry(void) {
    /* rack: 10 cups per side, mirrored in x, triangle rows of 1..4 */
    Vec3 a = party_rack_cup(0, 0), b = party_rack_cup(1, 0), c = party_rack_cup(1, 9), d = party_rack_cup(1, 6);
    CHECK_NEAR(a.x, -.45, 1e-12); CHECK_NEAR(b.x, .45, 1e-12); CHECK_NEAR(a.y, .64, 1e-12); CHECK_NEAR(a.z, 0, 1e-12);
    CHECK_NEAR(c.x, .888, 1e-12); CHECK_NEAR(c.z, .219, 1e-12); CHECK_NEAR(d.z, -.219, 1e-12);
    /* table -> world: rot.y = PI/2 maps local +x to world -z */
    Vec3 w = party_table_to_world((Vec3){1, .5, .25});
    CHECK_NEAR(w.x, -2.52, 1e-9); CHECK_NEAR(w.y, .5, 1e-12); CHECK_NEAR(w.z, -8.65, 1e-9);
    /* JS: launchPoints[0] = handPosition(pong-red) in the release pose (the ball's position at frame 71 = releaseAt) */
    party_init(&party, 1234);
    CHECK_NEAR(party.launch_points[0].x, -party.launch_points[1].x, 1e-9);
    CHECK_NEAR(party.launch_points[0].y, party.launch_points[1].y, 1e-9);
    CHECK_NEAR(party.swat_points[0].y, party.swat_points[1].y, 1e-9);
    /* JS: hose segment 9 midpoint with the tap at rest (hose.getMatrixAt(9) position) */
    Vec3 h = {(party.hose[9].x + party.hose[10].x) / 2, (party.hose[9].y + party.hose[10].y) / 2, (party.hose[9].z + party.hose[10].z) / 2};
    CHECK_NEAR(h.x, 0.410199, 2e-6); CHECK_NEAR(h.y, 0.530181, 2e-6); CHECK_NEAR(h.z, 0.361955, 2e-6);
    CHECK_NEAR(party.hose[0].x, .12, 1e-9); CHECK_NEAR(party.hose[0].y, .98, 1e-9);
    CHECK_NEAR(party.hose[PARTY_HOSE_SEGMENTS].y, .79, 1e-9); CHECK_NEAR(party.hose[PARTY_HOSE_SEGMENTS].z, .43, 1e-9);
    CHECK_NEAR(party_dog_head_yaw((Vec3){2.3, 0, -8.7}, 2.6), 1.947240, 2e-6);   /* JS: dog.head.rotation.y during the toss */
}

static void test_keg(void) {
    at(2.0);   /* approaching */
    CHECK(!party.fill && !party.drink && !party.keg_stand, "t=2 approach");
    CHECK_NEAR(party.amount, .5, 1e-12);
    CHECK_NEAR(party.guest_pos.x, 2.95 + .24, 1e-9); CHECK_NEAR(party.guest_pos.z, -6.15 - .19, 1e-9);
    CHECK_NEAR(party.guest_pos.y, sin(16.0) * .012, 1e-12);
    CHECK_NEAR(party.guest_rot_y, .5, 1e-12); CHECK_NEAR(party.guest_arm_x, -.15, 1e-12);
    CHECK(party.guest_cup_visible && !party.stream_visible && !party.serving_visible, "t=2 holds the cup");
    CHECK_NEAR(party.plunger_y, 1.295, 1e-12); CHECK_NEAR(party.squeeze_rot_x, -.35, 1e-12);
    at(3.99); CHECK(!party.fill, "fill starts at 4");
    at(4.0); CHECK(party.fill, "fill at 4");
    at(6.0);   /* filling */
    CHECK(party.fill && !party.drink, "t=6 fill");
    CHECK_NEAR(party.amount, 1, 1e-12); CHECK_NEAR(party.guest_pos.y, 0, 1e-12);
    CHECK_NEAR(party.guest_rot_y, -.45, 1e-12); CHECK_NEAR(party.guest_arm_x, -1.05, 1e-12); CHECK_NEAR(party.guest_arm_z, -.65, 1e-12);
    CHECK(!party.guest_cup_visible && party.stream_visible && party.serving_visible, "t=6 cup under the tap");
    CHECK_NEAR(party.plunger_y, 1.27 + .045 * sin(48.0), 1e-12); CHECK_NEAR(party.squeeze_rot_x, -.7, 1e-12);
    at(8.0); CHECK(!party.fill && !party.drink && party.guest_cup_visible, "fill ends at 8");
    at(8.99); CHECK(!party.drink, "drink starts at 9");
    at(11.0);  /* drinking */
    CHECK(party.drink && !party.fill, "t=11 drink");
    CHECK_NEAR(party.guest_rot_y, -.1, 1e-12); CHECK_NEAR(party.guest_arm_x, -2.4, 1e-12); CHECK_NEAR(party.guest_arm_z, -.15, 1e-12);
    at(13.0); CHECK(!party.drink, "drink ends at 13");
    at(15.0);  /* returning */
    CHECK_NEAR(party.amount, .5, 1e-12); CHECK_NEAR(party.guest_pos.y, sin(120.0) * .012, 1e-12);
    at(18.0); CHECK_NEAR(party.amount, 0, 1e-12); CHECK_NEAR(party.guest_pos.x, 2.95, 1e-9); CHECK_NEAR(party.guest_pos.z, -6.15, 1e-9);
    at(26.0); CHECK(party.fill, "the cycle repeats every 20 s");
    CHECK_NEAR(party.friend_pos.x, 3.05, 1e-9); CHECK_NEAR(party.friend_pos.z, -7.7, 1e-9);
    CHECK_NEAR(party.friend_rot_y, .8 + sin(26 * .3) * .13, 1e-12);
    CHECK_NEAR(party.friend_arm_x, -.3 - fmax(0, sin(26 * .45)) * .5, 1e-12);
    CHECK_NEAR(party.friend_arm_scale_y, 1, 1e-12);

    /* keg stand: occasion = time % 65 in [48, 57), 1.5 s ramps */
    at(47.99); CHECK(!party.keg_stand && party.stand == 0, "no stand before 48");
    at(48.0); CHECK(!party.keg_stand && party.stand == 0, "stand is 0 exactly at 48");
    at(48.75); CHECK(party.keg_stand, "ramping in"); CHECK_NEAR(party.stand, .5, 1e-9);
    CHECK_NEAR(party.guest_rot_x, PARTY_PI * .5, 1e-9);
    CHECK(!party.guest_cup_visible && !party.stream_visible && !party.serving_visible, "cups hidden during the stand");
    CHECK_NEAR(party.squeeze_rot_x, -.7, 1e-12);
    at(49.5); CHECK_NEAR(party.stand, 1, 1e-9);
    at(52.0);
    CHECK_NEAR(party.stand, 1, 1e-12);
    CHECK_NEAR(party.guest_pos.x, 3.7, 1e-9); CHECK_NEAR(party.guest_pos.y, 1.85, 1e-9); CHECK_NEAR(party.guest_pos.z, -6.67, 1e-9);
    CHECK_NEAR(party.guest_rot_x, PARTY_PI, 1e-12); CHECK_NEAR(party.guest_rot_y, 0, 1e-12);
    CHECK_NEAR(party.friend_pos.x, 3.25, 1e-9); CHECK_NEAR(party.friend_pos.z, -6.8, 1e-9); CHECK_NEAR(party.friend_rot_y, 0, 1e-12);
    CHECK_NEAR(party.friend_arm_z, 2.45, 1e-12); CHECK_NEAR(party.friend_arm_scale_y, 1.6, 1e-12);
    CHECK_NEAR(party.tap_pos.y, .88, 1e-12); CHECK_NEAR(party.tap_pos.z, .359, 1e-12);
    CHECK_NEAR(party.hose[PARTY_HOSE_SEGMENTS].y, .88, 1e-9); CHECK_NEAR(party.hose[PARTY_HOSE_SEGMENTS].z, .289, 1e-9);
    at(56.25); CHECK_NEAR(party.stand, .5, 1e-9);
    at(57.0); CHECK(!party.keg_stand && party.stand == 0, "stand ends at 57");
    at(65 + 52.0); CHECK_NEAR(party.stand, 1, 1e-12);
    double last = 0; int rises = 0;
    party_init(&party, 1234);
    for (int i = 0; i < 60 * 65; i++) {
        party_update(&party, 1.0 / 60);
        CHECK(party.stand >= 0 && party.stand <= 1, "stand in range");
        CHECK(fabs(party.stand - last) < 1.0 / 60 / 1.5 + 1e-9, "stand ramps without a jump at t=%.3f", party.time);
        if (party.stand > 0 && last == 0) rises++;
        last = party.stand;
    }
    CHECK(rises == 1, "one keg stand per 65 s (%d)", rises);
}

static void test_cook(void) {
    at(0.65);   /* mid flip of foods[0] */
    CHECK(party.flipping && party.food_index == 0, "t=.65 flipping food 0");
    CHECK_NEAR(party.flip, .5, 1e-9); CHECK_NEAR(party.food_lift, .25, 1e-9); CHECK_NEAR(party.food_rot_z, PARTY_PI, 1e-9);
    CHECK_NEAR(party.cook_arm_x, -2.2, 1e-9); CHECK_NEAR(party.cook_arm_z, -.5, 1e-12);
    at(1.31); CHECK(!party.flipping && party.food_lift == 0 && party.food_rot_z == 0, "flip lasts 1.3 s");
    CHECK_NEAR(party.cook_arm_x, -.8, 1e-12);
    at(8.5); CHECK(party.flipping && party.food_index == 1, "second food at 8 s");
    at(16.5); CHECK(party.food_index == 2, "third food");
    at(24.5); CHECK(party.food_index == 3, "fourth food");
    at(32.5); CHECK(party.food_index == 0, "wraps to the first food");
    bool steak;
    Vec3 f = party_food_base(2, &steak);
    CHECK(!steak, "food 2 is a burger"); CHECK_NEAR(f.x, 2.9, 1e-12); CHECK_NEAR(f.y, .75, 1e-12); CHECK_NEAR(f.z, -8.25, 1e-12);
    party_food_base(1, &steak); CHECK(steak, "food 1 is a steak");

    at(24.99); CHECK(!party.toss && party.catches == 0, "no toss before 25");
    at(25.0); CHECK(party.toss, "toss starts at 25"); CHECK_NEAR(party.toss_progress, 0, 1e-9);
    at(25.7);
    CHECK(party.toss && party.catches == 0, "mid toss");
    CHECK_NEAR(party.toss_progress, .5, 1e-9); CHECK_NEAR(party.snack_rot_z, 1.5 * PARTY_PI, 1e-9); CHECK_NEAR(party.cook_arm_x, -1.9, 1e-12);
    Vec3 mouth = {2.2, .4, -8.5}, snack = party_snack_position(&party, mouth);
    CHECK_NEAR(snack.x, (party.snack_from.x + mouth.x) / 2, 1e-9); CHECK_NEAR(snack.y, (party.snack_from.y + mouth.y) / 2 + .75, 1e-9);
    at(26.39); CHECK(party.toss && party.catches == 0, "toss until 26.4");
    at(26.4); CHECK(!party.toss && party.catches == 1, "caught at 26.4");
    at(36 + 25.7); CHECK(party.toss, "toss repeats every 36 s");

    /* catches increments exactly once per 36 s cycle */
    party_init(&party, 1234);
    int last = 0;
    for (int i = 1; i <= 60 * 36 * 5; i++) {
        party_update(&party, 1.0 / 60);
        CHECK(party.catches == last || party.catches == last + 1, "catches step");
        if (party.catches != last) CHECK(party.cook_cycle >= 26.4 && party.cook_cycle < 26.4 + 2.0 / 60, "catch lands at cycle %.3f", party.cook_cycle);
        last = party.catches;
    }
    CHECK(party.catches == 5, "five catches in five cycles (%d)", party.catches);
}

static void test_pong_flow(void) {
    party_init(&party, 1234);
    CHECK(party.match.id == 0 && party.matches_started == 1 && party.turn == 0, "seed 1234 opens with red-blowout");   /* JS */
    CHECK(party.remaining[0] == 10 && party.remaining[1] == 10 && party.winner == -1, "fresh racks");
    CHECK(party.ball_visible, "ball in hand at t=0");
    /* JS frame 91 (t = 91/60): ball in flight on red's opening hit */
    run_frames(&party, 91);
    CHECK(party.ball_visible && party.turn == 0, "frame 91 in flight");
    CHECK_NEAR(party.ball_local.x, -0.707695, 2e-6); CHECK_NEAR(party.ball_local.y, 1.162570, 2e-6); CHECK_NEAR(party.ball_local.z, -0.033778, 2e-6);
    CHECK_NEAR(party.ball_world.x, -2.803778, 2e-6); CHECK_NEAR(party.ball_world.y, 1.162570, 2e-6); CHECK_NEAR(party.ball_world.z, -6.942305, 2e-6);
    CHECK_NEAR(party.players[0].shoulder, -0.329158, 2e-6); CHECK_NEAR(party.players[0].elbow, -0.030271, 2e-6);
    CHECK_NEAR(party.players[1].shoulder, -.2, 1e-12);
    /* JS frame 1533 (t = 25.55): turn 6, remaining 9/8, burger mid-air */
    run_frames(&party, 1533 - 91);
    CHECK(party.turn == 6 && party.remaining[0] == 9 && party.remaining[1] == 8, "frame 1533 turn %d remaining %d/%d", party.turn, party.remaining[0], party.remaining[1]);
    CHECK_NEAR(party.ball_local.x, -0.928382, 2e-6); CHECK_NEAR(party.ball_local.y, 1.030703, 2e-6); CHECK_NEAR(party.ball_local.z, -0.036616, 2e-6);
    CHECK_NEAR(party.players[0].shoulder, -1.964180, 2e-6); CHECK_NEAR(party.players[0].elbow, -0.413480, 2e-6);
    CHECK_NEAR(party.plunger_y, 1.261215, 2e-6);
    CHECK(party.toss, "frame 1533 toss"); CHECK_NEAR(party.snack_rot_z, 3.702591, 2e-6);
    /* JS frame 3003 (t = 50.05): keg stand at full blend, turn 12, remaining 8/5 */
    run_frames(&party, 3003 - 1533);
    CHECK(party.turn == 12 && party.remaining[0] == 8 && party.remaining[1] == 5 && party.keg_stand, "frame 3003 turn %d remaining %d/%d", party.turn, party.remaining[0], party.remaining[1]);
    CHECK_NEAR(party.ball_local.x, -0.340759, 2e-6); CHECK_NEAR(party.ball_local.y, 1.058838, 2e-6); CHECK_NEAR(party.ball_local.z, 0.011492, 2e-6);
    CHECK_NEAR(party.players[0].shoulder, -0.676580, 2e-6);
    CHECK(party.catches == 1, "one catch by t=50");

    /* Whole first match + celebration, then the loop restarts with a different script. */
    party_init(&party, 1234);
    PongMatch first = party.match;
    int remaining[2] = {10, 10}, last_turn = 0;
    bool cheered = false;
    int frames = (int)ceil(first.loop_seconds * 60) + 2;
    for (int i = 0; i < frames && party.matches_started == 1; i++) {
        party_update(&party, 1.0 / 60);
        if (party.matches_started != 1) break;
        CHECK(party.turn >= last_turn && party.turn < first.turn_count, "turn advances (%d -> %d)", last_turn, party.turn);
        last_turn = party.turn;
        for (int s = 0; s < 2; s++) {
            CHECK(party.remaining[s] <= remaining[s] && party.remaining[s] >= remaining[s] - 1 && party.remaining[s] >= 0, "remaining[%d] decreases monotonically (%d -> %d)", s, remaining[s], party.remaining[s]);
            remaining[s] = party.remaining[s];
            int visible = 0;
            for (int c = 0; c < PONG_CUPS; c++) { if (party.cup_visible[s][c]) visible++; CHECK(party.cup_visible[s][c] == (c >= PONG_CUPS - party.remaining[s]), "cups sink in rack order"); }
            CHECK(visible == party.remaining[s], "visible cups match remaining");
        }
        CHECK(party.remaining[0] == 10 - party.sunk[1] && party.remaining[1] == 10 - party.sunk[0], "remaining mirrors sunk");
        CHECK(party.ball_visible == (!party.finished && party.phase < PONG_IMPACT_AT), "ball visibility rule");
        if (party.phase >= PONG_IMPACT_AT) CHECK(!party.ball_visible, "ball hidden after impact");
        if (party.finished) {
            cheered = true;
            CHECK(!party.ball_visible, "ball hidden when finished");
            CHECK(party.winner == first.winner, "winner %d", party.winner);
            CHECK(party.players[party.winner].cheering && !party.players[1 - party.winner].cheering, "only the winner cheers");
            CHECK(party.players[party.winner].y >= 0 && party.players[party.winner].y <= .13 + 1e-12, "cheer hop height");
            CHECK(party.players[party.winner].shoulder <= -2.52 + 1e-9 && party.players[party.winner].shoulder >= -2.8 - 1e-9, "cheer arm raised");
            CHECK(party.remaining[1 - party.winner] == 0, "loser's rack is empty");
        } else {
            CHECK(party.winner == -1 && !party.players[0].cheering && !party.players[1].cheering, "no winner before the last impact");
            CHECK(party.players[0].y == 0 && party.players[1].y == 0, "nobody hops mid-match");
        }
        if (party.ball_visible) {
            CHECK(fabs(party.ball_local.x) < 1.6 && party.ball_local.y > 0 && party.ball_local.y < 1.6 && fabs(party.ball_local.z) < 1.0, "ball stays near the table (%.3f %.3f %.3f)", party.ball_local.x, party.ball_local.y, party.ball_local.z);
            Vec3 w = party_table_to_world(party.ball_local);
            CHECK(NEAR(w.x, party.ball_world.x, 1e-12) && NEAR(w.y, party.ball_world.y, 1e-12) && NEAR(w.z, party.ball_world.z, 1e-12), "ball world = table transform");
            if (party.phase < PONG_RELEASE_AT) {
                Vec3 hand = party_pong_hand(party.match.turns[party.turn].side, &party.players[party.match.turns[party.turn].side]);
                CHECK(NEAR(hand.x, party.ball_local.x, 1e-12) && NEAR(hand.y, party.ball_local.y, 1e-12) && NEAR(hand.z, party.ball_local.z, 1e-12), "ball rides the shooter's hand before release");
            }
        }
    }
    CHECK(cheered, "the celebration was observed");
    CHECK(party.matches_started == 2, "loop restarted (%d)", party.matches_started);
    CHECK(party.match.id != first.id, "the next match is a different script");
    CHECK(party.match.id == 3, "JS: seed 1234 second match is blue-solid (got %d)", party.match.id);
    CHECK_NEAR(party.match_start, first.loop_seconds, 1e-9);
    CHECK(party.remaining[0] == 10 && party.remaining[1] == 10 && party.winner == -1 && party.turn == 0, "racks reset");
    CHECK(party.time - party.match_start < 2.0 / 60, "restart happens on the loop boundary");

    /* Shot paths: swats deflect off the defender's hand, bounces touch the table first. */
    for (int id = 0; id < PONG_CATALOG_COUNT; id++) {
        for (uint32_t seed = 1; seed < 200; seed++) {
            party_init(&party, seed);
            if (party.match.id != id) continue;
            for (int i = 0; i < party.match.turn_count; i++) {
                const PongTurn *t = &party.match.turns[i]; const PongShot *s = &party.shots[i];
                CHECK(s->has_contact == (t->result == PONG_SWAT), "contact only on swats");
                if (t->result == PONG_HIT) { CHECK(!s->has_bounce, "hits fly direct"); CHECK_NEAR(s->target.y, .77, 1e-12); }
                if (t->result == PONG_BOUNCE || t->result == PONG_SWAT) { CHECK(s->has_bounce, "bounce point"); CHECK_NEAR(s->bounce.y, .672, 1e-12); CHECK_NEAR(fabs(s->bounce.x), .43, 1e-12); }
                if (t->result == PONG_SWAT) { CHECK_NEAR(s->contact.x, party.swat_points[1 - t->side].x, 1e-12); CHECK_NEAR(s->target.y, s->contact.y - .37, 1e-12); }
                CHECK((s->miss_type != PONG_MISS_NONE) == (t->result == PONG_MISS), "miss type only on misses");
                if (t->result == PONG_MISS) CHECK(s->has_bounce == (s->miss_type == PONG_MISS_RIM), "rim misses bounce");
                if (t->result == PONG_HIT || t->result == PONG_BOUNCE) {
                    Vec3 cup = party_rack_cup(1 - t->side, t->before[t->side]);
                    CHECK(NEAR(s->target.x, cup.x, 1e-12) && NEAR(s->target.z, cup.z, 1e-12), "scoring shots land in the next standing cup");
                }
            }
            break;
        }
    }
}

static bool finite3(Vec3 v) { return isfinite(v.x) && isfinite(v.y) && isfinite(v.z); }
static bool all_finite(const Party *p) {
    bool ok = isfinite(p->time) && isfinite(p->stand) && isfinite(p->amount) && finite3(p->guest_pos) && finite3(p->friend_pos)
        && isfinite(p->guest_rot_x) && isfinite(p->guest_rot_y) && isfinite(p->guest_arm_x) && isfinite(p->guest_arm_z)
        && isfinite(p->friend_rot_y) && isfinite(p->friend_arm_x) && isfinite(p->friend_arm_z) && isfinite(p->friend_arm_scale_y)
        && isfinite(p->plunger_y) && finite3(p->tap_pos) && isfinite(p->squeeze_rot_x) && isfinite(p->phase) && isfinite(p->match_start)
        && isfinite(p->cook_arm_x) && isfinite(p->food_lift) && isfinite(p->food_rot_z) && isfinite(p->toss_progress) && isfinite(p->snack_rot_z);
    for (int i = 0; i <= PARTY_HOSE_SEGMENTS; i++) ok = ok && finite3(p->hose[i]);
    for (int s = 0; s < 2; s++) ok = ok && isfinite(p->players[s].y) && isfinite(p->players[s].shoulder) && isfinite(p->players[s].elbow) && isfinite(p->players[s].sweep);
    if (p->ball_visible) ok = ok && finite3(p->ball_local) && finite3(p->ball_world);
    if (p->toss) ok = ok && finite3(p->snack_from);
    return ok;
}

static void test_soak(void) {
    static Party a, b, c;
    party_init(&a, 1234); party_init(&b, 1234); party_init(&c, 99);
    bool finite = true, same = true, no_repeat = true, differs = false;
    int previous = a.match.id, started = 1;
    for (int i = 0; i < 60 * 60 * 30; i++) {
        party_update(&a, 1.0 / 60); party_update(&b, 1.0 / 60); party_update(&c, 1.0 / 60);
        if (!all_finite(&a) || !all_finite(&c)) finite = false;
        if (a.match.id != b.match.id || a.turn != b.turn || a.time != b.time || a.catches != b.catches || a.ball_visible != b.ball_visible
            || (a.ball_visible && (a.ball_world.x != b.ball_world.x || a.ball_world.y != b.ball_world.y || a.ball_world.z != b.ball_world.z))
            || a.guest_pos.x != b.guest_pos.x || a.players[0].shoulder != b.players[0].shoulder || a.rng.state != b.rng.state) same = false;
        if (a.matches_started != started) { started = a.matches_started; if (a.match.id == previous) no_repeat = false; previous = a.match.id; }
        if (a.match.id != c.match.id) differs = true;
        if (a.turn < 0 || a.turn >= a.match.turn_count || a.remaining[0] < 0 || a.remaining[1] < 0) finite = false;
    }
    CHECK(finite, "30 simulated minutes stay finite and in range");
    CHECK(same, "same seed -> identical run");
    CHECK(no_repeat, "no script plays twice in a row");
    CHECK(differs, "a different seed picks different scripts");
    CHECK_NEAR(a.time, 1800, 1e-6);
    /* JS: createParty with mulberry32(1234) after 108000 updates of 1/60: 15th match, blue-close, 50 catches */
    CHECK(a.matches_started == 15 && a.match.id == 1, "JS: 15 matches, blue-close running (got %d, id %d)", a.matches_started, a.match.id);
    CHECK(a.catches == 50, "JS: 50 catches in 30 minutes (got %d)", a.catches);
    /* A long frame (e.g. after a pause) still lands in a valid state. */
    party_update(&a, 1000.0);
    CHECK(all_finite(&a) && a.turn >= 0 && a.turn < a.match.turn_count && a.time - a.match_start < a.match.loop_seconds, "large dt catches up through several matches");
    party_update(&a, 0);
    CHECK(all_finite(&a), "zero dt is harmless");
}

int main(void) {
    test_catalog();
    test_select_next();
    test_miss_paths();
    test_geometry();
    test_keg();
    test_cook();
    test_pong_flow();
    test_soak();
    printf("party tests: %d/%d passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
