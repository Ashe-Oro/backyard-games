/* Host tests for the Wii game layer: match flow, scoring, modes, career, wind policy, menus,
 * pause/disconnect, saves and handedness. Drives the real Input layer through its public API. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"

static int failures, checks;
#define CHECK(cond, ...) do { checks++; if (!(cond)) { failures++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static WinTable win;
static Input input;
static Game game;
static InputEvent queue[64];
static int queued;
static void sink(void *user, const InputEvent *e) { (void)user; if (queued < 64) queue[queued++] = *e; }
static void drain(void) { for (int i = 0; i < queued; i++) game_input(&game, &queue[i]); queued = 0; }
static uint64_t now_ms = 1000;
static void frames(int n) { for (int i = 0; i < n; i++) { now_ms += 16; drain(); game_update(&game, 1.0 / 60, now_ms); drain(); } }
static void event(InputAction a, double value) { InputEvent e; memset(&e, 0, sizeof e); e.action = a; e.value = value; game_input(&game, &e); drain(); }
static void boot(uint32_t seed, const SaveData *save) {
    input_init(&input, sink, NULL);
    input_connection(&input, 0, true);
    queued = 0;
    game_init(&game, &input, &win, seed, save, true);
    drain();
}
static int menu_index(MenuItemId id) { for (int i = 0; i < game.menu.count; i++) if (game.menu.items[i].id == id) return i; return -1; }
static void feed_still(unsigned ch, bool b) {
    InputSample s; memset(&s, 0, sizeof s);
    s.motion.ms = now_ms; s.motion.g = (SwingVector){0, 0, 1}; s.motion.accel_valid = true;
    s.buttons_valid = true; s.buttons = b ? INPUT_B : 0; s.motion.b = b;
    input_feed(&input, ch, &s); drain();
}

/* Plays a whole match by throwing for every human turn. Returns frames used, or -1 on soft-lock. */
static int play_match(double power_mid, int max_frames) {
    int f = 0;
    while (f < max_frames) {
        if (game.ui == UI_SUMMARY && game.phase == PH_SUMMARY) game_menu_select(&game, MI_NEXT);
        if (game.phase == PH_OVER) return f;
        if (game.phase == PH_AIM && game.ui == UI_NONE && game_is_human(&game, game_thrower(&game))) {
            double p = power_mid + (rng_next(&game.rng) - .5) * 0.18;
            double aim = (rng_next(&game.rng) - .5) * 0.06, spin = (rng_next(&game.rng) - .5) * 1.2;
            game.heights[game_thrower(&game)] = 1; game.arc = ARC_NORMAL;
            CHECK(game_throw(&game, p, aim, spin), "throw accepted in AIM");
        }
        frames(1); f++;
    }
    return -1;
}

static void test_boot_and_menus(void) {
    boot(1, NULL);
    CHECK(game.ui == UI_STRAP, "boot shows STRAP ON!");
    event(INPUT_CONFIRM, 0);
    CHECK(game.ui == UI_TITLE, "OK goes to the title menu");
    CHECK(game.menu.count == 5 && game.menu.items[0].id == MI_PRACTICE && game.menu.items[1].id == MI_TWO_PLAYER &&
          game.menu.items[2].id == MI_TOURNAMENT && game.menu.items[3].id == MI_SETTINGS && game.menu.items[4].id == MI_QUIT,
          "title: PRACTICE / 2 PLAYER / TOURNAMENT / SETTINGS / QUIT");
    frames(1);
    CHECK(input.context == INPUT_MENU, "menus use INPUT_MENU");
    event(INPUT_NAV_DOWN, 0); event(INPUT_NAV_DOWN, 0); event(INPUT_NAV_DOWN, 0); event(INPUT_NAV_DOWN, 0);
    event(INPUT_CONFIRM, 0);
    CHECK(game.ui == UI_CONFIRM_QUIT, "QUIT asks to confirm");
    event(INPUT_BACK, 0);
    CHECK(game.ui == UI_TITLE && !game.quit_requested, "B cancels quit");
    game_menu_select(&game, MI_QUIT); game_menu_select(&game, MI_YES);
    CHECK(game.quit_requested, "YES quits to the Homebrew Channel");
}

static void test_two_player_soak(void) {
    boot(7, NULL); game_menu_select(&game, MI_OK);
    int rounds = 0, matches = 0;
    while (rounds < 20) {
        game_start(&game, false, false, false);
        CHECK(game.wind.mph == 0, "2 PLAYER defaults to calm air");
        int f = play_match(0.70, 60 * 60 * 40);
        CHECK(f >= 0, "2 PLAYER match finishes without a soft-lock");
        if (f < 0) return;
        CHECK(game.winner >= 0 && game.scores[game.winner] >= 21, "winner reached 21 (%d-%d)", game.scores[0], game.scores[1]);
        CHECK(game.round.scorer == game.winner, "last round scorer won");
        rounds += game.inning; matches++;
        game_menu_select(&game, MI_NEXT);
        CHECK(game.ui == UI_TITLE && game.phase == PH_MENU, "NEW MATCH returns to the menu");
    }
    printf("  2 PLAYER soak: %d matches, %d rounds\n", matches, rounds);
}

static void test_practice(void) {
    boot(11, NULL); game_menu_select(&game, MI_OK);
    game_start(&game, true, false, false);
    int throws = 0, f = 0;
    while (throws < 200 && f < 200 * 600) {
        if (game.phase == PH_AIM && game.ui == UI_NONE) {
            CHECK(game_thrower(&game) == 0, "practice thrower is RED");
            game_throw(&game, 0.66 + (rng_next(&game.rng) - .5) * 0.2, 0, 0); throws++;
            if (throws % 40 == 0) {
                frames(240);
                int in = game.practice_in, on = game.practice_on, n = game.practice_throws;
                event(INPUT_PAUSE, 0);
                CHECK(game.ui == UI_PAUSE && menu_index(MI_CLEAR_BOARD) == 1, "practice pause offers CLEAR BOARD");
                game_menu_select(&game, MI_CLEAR_BOARD);
                CHECK(game.nbags == 0 && game.practice_in == in && game.practice_on == on && game.practice_throws == n,
                      "CLEAR BOARD removes bags and keeps stats");
            }
        }
        frames(1); f++;
    }
    CHECK(throws == 200, "200 practice throws without a soft-lock (%d)", throws);
    CHECK(game.practice_throws >= 199, "practice stats counted (%d)", game.practice_throws);
    CHECK(game.practice_in + game.practice_on > 20, "practice throws reach the board (%d in, %d on)", game.practice_in, game.practice_on);
}

static void test_career(void) {
    boot(21, NULL); game_menu_select(&game, MI_OK);
    for (int level = 0; level < 8; level += 7) {
        game.fight = level; game.wins = level;
        game_menu_select(&game, MI_TOURNAMENT);
        CHECK(game.ui == UI_CAREER_MAP, "TOURNAMENT opens the circuit map");
        game_menu_select(&game, MI_CONTINUE);
        CHECK(game.ui == UI_INTRO && game.phase == PH_INTRO, "CONTINUE shows the rival intro");
        game_menu_select(&game, MI_LETS_TOSS);
        CHECK(game.career_match && game.vs_cpu, "LET'S TOSS starts a career match");
        bool saw_cpu = false;
        int f = 0;
        while (game.phase != PH_OVER && f < 60 * 60 * 60) {
            if (game.phase == PH_CPU) saw_cpu = true;
            if (game.ui == UI_SUMMARY && game.phase == PH_SUMMARY) game_menu_select(&game, MI_NEXT);
            if (game.phase == PH_AIM && game.ui == UI_NONE && game_thrower(&game) == 0) game_throw(&game, 0.70, 0, 0);
            frames(1); f++;
        }
        CHECK(game.phase == PH_OVER, "career match vs %s finishes", byc_roster[level].name);
        CHECK(saw_cpu, "rival took CPU turns");
        CHECK(game.ui == UI_CAREER_RESULT, "career result panel shown");
        int expect = level + (game.winner == 0 ? 1 : 0);
        CHECK(game.fight == expect, "fight advances only on a win (%d vs %d)", game.fight, expect);
        CHECK(!game.winner || game.save_dirty, "a won fight is saved");
        printf("  career vs %s: %d-%d after %d rounds, plan steps %ld\n", byc_roster[level].name, game.scores[0], game.scores[1], game.inning, game.plan.steps);
        game_menu_select(&game, MI_CIRCUIT_MAP);
        CHECK(game.ui == UI_CAREER_MAP, "CIRCUIT MAP returns to the map");
        game_menu_select(&game, MI_MAIN_MENU);
    }
    game.fight = 8; game.wins = 9;
    game_menu_select(&game, MI_TOURNAMENT);
    CHECK(game.menu.count && strcmp(game.menu.items[0].label, "VIEW TROPHY") == 0, "cleared career offers VIEW TROPHY");
    game_menu_select(&game, MI_CONTINUE);
    CHECK(game.ui == UI_CHAMPION, "trophy screen");
    game_menu_select(&game, MI_NEW_CAREER);
    CHECK(game.ui == UI_CONFIRM_NEW_CAREER, "NEW CAREER asks to confirm");
    game_menu_select(&game, MI_YES);
    CHECK(game.fight == 0 && game.wins == 0 && game.save.career_fight == 0, "NEW CAREER resets to 0/0 and saves");
}

static Bag placed(int team, double x, double z) {
    Bag b; bag_new(&b, team, SIDE_SLICK); b.lp = (Vec3){x, BYC_BAG_H, z}; b.p = board_to_world(b.lp); b.state = BAG_BOARD; b.touched = true;
    return b;
}
static void test_scoring(void) {
    boot(31, NULL); game_menu_select(&game, MI_OK);
    game_start(&game, false, false, false);
    double hz = byc_board.hole_local.z;
    /* RED 6 (1 in + 3 on) vs BLUE 4 (1 in + 1 on) -> RED +2, scored once after settle */
    game.nbags = 0;
    game.bags[game.nbags++] = placed(0, 0.1, hz + 0.3); game.bags[game.nbags++] = placed(0, -0.1, hz + 0.35);
    game.bags[game.nbags++] = placed(0, 0.15, hz - 0.2); game.bags[game.nbags - 1].state = BAG_BOARD;
    Bag in0 = placed(0, 0, hz); in0.state = BAG_IN; game.bags[game.nbags++] = in0;
    Bag in1 = placed(1, 0, hz); in1.state = BAG_IN; game.bags[game.nbags++] = in1;
    game.bags[game.nbags++] = placed(1, -0.2, hz + 0.5);
    Bag grass = placed(1, 1, -3); grass.state = BAG_GRASS; grass.dead = true; grass.rest_t = 0.8; grass.state = BAG_REMOVED; game.bags[game.nbags++] = grass;
    Bag moving = placed(1, 0.2, hz + 0.6); moving.state = BAG_SLIDE; game.bags[game.nbags++] = moving;
    Lie live = game_lie(&game);
    CHECK(live.pts[0] == 6 && live.pts[1] == 4, "lie(): moving slides and removed grounders don't count (%d-%d)", live.pts[0], live.pts[1]);
    game.timers[0].kind = T_NONE; memset(game.timers, 0, sizeof game.timers);
    game.k = 8; game.phase = PH_SETTLE; game.first = 1;
    for (int i = 0; i < 600 && game.phase == PH_SETTLE; i++) frames(1);
    CHECK(game.phase == PH_SUMMARY, "round scored after every bag settles");
    CHECK(game.round.net == game.round.lie.pts[0] - game.round.lie.pts[1] && game.round.scorer == 0, "cancellation scoring to RED");
    CHECK(game.scores[0] == game.round.net && game.scores[1] == 0 && game.first == 0, "RED scores the net and throws first");
    /* wash keeps the order */
    game_menu_select(&game, MI_NEXT); frames(2);
    memset(game.timers, 0, sizeof game.timers);
    game.nbags = 0; game.bags[game.nbags++] = placed(0, 0.1, hz + 0.3); game.bags[game.nbags++] = placed(1, -0.1, hz + 0.3);
    game.k = 8; game.phase = PH_SETTLE; int first = game.first, s0 = game.scores[0];
    frames(5);
    CHECK(game.round.scorer == -1 && game.first == first && game.scores[0] == s0, "WASH keeps throw order and scores");
    /* crossing 21 ends the match */
    game_menu_select(&game, MI_NEXT); frames(2); memset(game.timers, 0, sizeof game.timers);
    game.scores[1] = 19; game.nbags = 0;
    Bag in2 = placed(1, 0, hz); in2.state = BAG_IN; game.bags[game.nbags++] = in2;
    game.k = 8; game.phase = PH_SETTLE; frames(5);
    CHECK(game.phase == PH_OVER && game.winner == 1 && game.scores[1] == 22, "a net score crossing 21 ends the match (%d)", game.scores[1]);
}

static void test_wind_policy(void) {
    SaveData s; save_defaults(&s); s.wind_on = true;
    boot(41, &s); game_menu_select(&game, MI_OK);
    game_start(&game, false, false, false);
    CHECK(game.wind.mph >= 6 && game.wind.mph <= 10, "WIND ON: 2 PLAYER gets 6-10 mph (%g)", game.wind.mph);
    double x1 = game.wind.x;
    int f = 0;
    while (game.inning == 1 && f < 60 * 120) {
        if (game.ui == UI_SUMMARY && game.phase == PH_SUMMARY) game_menu_select(&game, MI_NEXT);
        if (game.phase == PH_AIM && game.ui == UI_NONE) game_throw(&game, 0.7, 0, 0);
        frames(1); f++;
    }
    frames(2);
    CHECK(game.inning == 2 && game.wind.mph > 0 && (game.wind.x > 0) != (x1 > 0), "next round flips the wind side");
    game_start(&game, true, false, false);
    CHECK(game.wind.mph >= 6, "WIND ON practice uses non-practice wind (%g)", game.wind.mph);
    game.save.wind_on = false;
    game_start(&game, true, false, false);
    CHECK(game.wind.mph == 0, "WIND CALM practice is calm");
    game.fight = 3; game_start(&game, false, true, true);
    CHECK(game.wind.mph >= 6 && game.wind.mph <= 9, "tournament County Fair circuit wind 6-9 mph regardless of setting (%g)", game.wind.mph);
}

static void test_human_throw_straight_from_stance(void) {
    boot(51, NULL); game_menu_select(&game, MI_OK);
    game_start(&game, true, false, false);
    while (game.phase != PH_AIM) frames(1);
    game.position[0] = 0.3;
    input.slots[0].stance = input.slots[0].stance_from = 0.3;
    frames(1);
    CHECK(fabs(game.position[0] - 0.3) < 1e-9, "stance read from the input layer");
    CHECK(game_throw(&game, 0.6, 0, 0), "throw");
    Bag *b = &game.bags[game.nbags - 1];
    CHECK(fabs(b->v.x) < 1e-12 && fabs(b->p.x - 0.3) < 1e-12, "straight swing launches straight from the stance (no auto-aim): vx=%g", b->v.x);
    frames(300);
    CHECK(fabs(b->p.x - 0.3) < 0.02 || b->state == BAG_REMOVED, "lands down-range of the stance (x=%g)", b->p.x);
    /* KOBE human power uses 1.0.3's remapped lob window */
    while (game.phase != PH_AIM) frames(1);
    game.heights[0] = 2; game.arc = ARC_KOBE;
    event(INPUT_HEIGHT, 2);
    CHECK(game.arc == ARC_KOBE, "D-pad height selects KOBE");
    game_throw(&game, 0.5, 0.02, -0.5);
    Bag *k = &game.bags[game.nbags - 1];
    double expect = physical_power(&win, 0, SIDE_SLICK, ARC_KOBE, 0.5);
    double pwr = expect, speed = pwr < 0.4 ? BYC_V_MIN + pwr * 6 : 8 + (pwr - 0.4) * 3.2;
    double v = speed * sqrt(sin(2 * BYC_ELEV) / sin(2 * 58 * BYC_PI / 180));
    CHECK(fabs(hypot(hypot(k->v.x, k->v.y), k->v.z) - v) < 1e-9, "KOBE throw uses physicalPower remap");
    CHECK(fabs(k->english + 0.5) < 1e-12 && k->spin == -0.5 * BYC_AIR_SPIN_RATE, "swing spin feeds bag spin (english/curl)");
}

static void test_pause_and_disconnect(void) {
    boot(61, NULL); game_menu_select(&game, MI_OK);
    game_start(&game, false, false, false);
    while (game.phase != PH_AIM) frames(1);
    frames(1);
    CHECK(input.context == INPUT_PLAY, "human aim uses INPUT_PLAY");
    game_throw(&game, 0.7, 0, 0);
    frames(5);
    Vec3 p = game.bags[0].p;
    event(INPUT_PAUSE, 0);
    CHECK(game.ui == UI_PAUSE && game.paused, "HOME pauses");
    frames(30);
    CHECK(game.bags[0].p.z == p.z, "pause freezes the bag in flight");
    event(INPUT_CONFIRM, 0);
    CHECK(game.ui == UI_NONE && !game.paused, "RESUME");
    frames(400);
    while (game.phase != PH_AIM) frames(1);
    /* Remote 1 disconnects on its turn: pause, prompt, resume only after reconnect + B up */
    int t = game_thrower(&game);
    input_connection(&input, input.channels[t], false); drain();
    CHECK(game.connect_prompt && game.connect_remote == input.channels[t] + 1, "CONNECT REMOTE %u prompt", game.connect_remote);
    int bags_before = game.nbags;
    CHECK(!game_throw(&game, 0.7, 0, 0) && game.nbags == bags_before && game.phase == PH_AIM, "no throw while the CONNECT REMOTE prompt freezes play");
    frames(30);
    CHECK(game.connect_prompt, "stays paused while disconnected");
    input_connection(&input, input.channels[t], true); drain();
    frames(1);
    CHECK(game.connect_prompt, "reconnect alone does not resume (B state unknown)");
    feed_still(input.channels[t], false);
    frames(1);
    CHECK(!game.connect_prompt, "resumes after reconnect + B up");
}

static void test_handedness_and_settings(void) {
    boot(71, NULL); game_menu_select(&game, MI_OK);
    game_menu_select(&game, MI_SETTINGS);
    CHECK(game.ui == UI_SETTINGS, "settings screen");
    CHECK(input.hand_option && input.hand_slot == 0, "RED HAND row focuses the input hand option");
    InputSample s; memset(&s, 0, sizeof s); s.buttons_valid = true; s.buttons = INPUT_LEFT; s.motion.g = (SwingVector){0, 0, 1};
    input_feed(&input, 0, &s); drain();
    CHECK(input.slots[0].profile.left_handed && game.save.profile[0].left_handed && game.save_dirty, "D-pad LEFT picks LEFT hand for RED and saves");
    s.buttons = 0; input_feed(&input, 0, &s);
    event(INPUT_NAV_DOWN, 0); event(INPUT_NAV_DOWN, 0); event(INPUT_NAV_DOWN, 0); event(INPUT_NAV_DOWN, 0);
    CHECK(game.menu.items[game.menu.sel].id == MI_HAND_BLUE && input.hand_slot == 1, "BLUE HAND row focuses slot 2");
    event(INPUT_CONFIRM, 0);
    CHECK(input.slots[1].profile.left_handed && !input.slots[0].profile.left_handed == false, "A toggles BLUE to LEFT");
    event(INPUT_CONFIRM, 0);
    CHECK(!input.slots[1].profile.left_handed, "A toggles BLUE back to RIGHT");
    double before = input.slots[0].profile.full_speed;
    game.menu.sel = menu_index(MI_POWER_RED); event(INPUT_ADJUST_PLUS, 0);
    CHECK(input.slots[0].profile.full_speed > before, "RED POWER + raises full-power speed");
    game.menu.sel = menu_index(MI_CURL_RED); event(INPUT_ADJUST_MINUS, 0);
    CHECK(fabs(input.slots[0].profile.spin_strength - 0.9) < 1e-9, "RED CURL - lowers curl strength");
    game.menu.sel = menu_index(MI_WIND); event(INPUT_CONFIRM, 0);
    CHECK(game.save.wind_on, "WIND toggles ON");
    game.menu.sel = menu_index(MI_SETPOWER_RED); event(INPUT_CONFIRM, 0);
    frames(2);
    CHECK(game.ui == UI_SETPOWER && input.calibrating && input.context == INPUT_PLAY, "SET POWER starts input calibration");
    event(INPUT_PAUSE, 0);
    CHECK(game.ui == UI_SETTINGS && !input.calibrating, "HOME cancels SET POWER");
    game.menu.sel = menu_index(MI_RESET_SETTINGS); event(INPUT_CONFIRM, 0);
    CHECK(!input.slots[0].profile.left_handed && !game.save.wind_on && input.slots[0].profile.spin_strength == 1, "RESET SETTINGS");
    /* pause menu exposes HAND too */
    game_start(&game, false, false, false);
    event(INPUT_PAUSE, 0);
    CHECK(menu_index(MI_HAND_RED) >= 0 && menu_index(MI_HAND_BLUE) >= 0, "pause offers RED HAND and BLUE HAND");
}

static void test_save_roundtrip(void) {
    SaveData s, d; save_defaults(&s);
    s.career_fight = 5; s.career_wins = 7; s.profile[1].left_handed = true; s.profile[0].full_speed = 3.1;
    s.profile[1].spin_strength = 0.4; s.help_tucked = true; s.wind_on = true;
    uint8_t buf[SAVE_MAX_BYTES]; size_t n = save_encode(&s, buf, sizeof buf);
    CHECK(n > 12 && memcmp(buf, "BYCS", 4) == 0 && buf[4] == 0 && buf[5] == SAVE_VERSION, "magic + big-endian version");
    CHECK(save_decode(&d, buf, n) && d.career_fight == 5 && d.career_wins == 7 && d.profile[1].left_handed &&
          d.profile[0].full_speed == 3.1 && d.profile[1].spin_strength == 0.4 && d.help_tucked && d.wind_on, "round trip");
    buf[8] ^= 1;
    CHECK(!save_decode(&d, buf, n) && d.career_fight == 0 && !d.wind_on, "CRC failure falls back to defaults");
    s.career_fight = 6; s.career_wins = 2; n = save_encode(&s, buf, sizeof buf);
    CHECK(save_decode(&d, buf, n) && d.career_fight == 0 && d.career_wins == 0 && d.profile[1].left_handed, "invalid career field defaults alone");
    CHECK(save_crc32((const uint8_t *)"123456789", 9) == 0xCBF43926u, "CRC-32 check value");
}

int main(void) {
    physics_init();
    win_compute(&win);
    test_boot_and_menus();
    test_save_roundtrip();
    test_scoring();
    test_wind_policy();
    test_human_throw_straight_from_stance();
    test_pause_and_disconnect();
    test_handedness_and_settings();
    test_practice();
    test_two_player_soak();
    test_career();
    printf("game tests: %d/%d checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
