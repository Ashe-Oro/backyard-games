/* Host tests for the Wii game layer: match flow, scoring, modes, career, wind policy, menus,
 * pause/disconnect, saves and handedness. Drives the real Input layer through its public API. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "bowling_trace.h"
#include "party.h"

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
    game_free(&game);
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
    CHECK(game.menu.count == 6 && game.menu.items[0].id == MI_PRACTICE && game.menu.items[1].id == MI_TWO_PLAYER &&
          game.menu.items[2].id == MI_TOURNAMENT && game.menu.items[3].id == MI_MUSIC && game.menu.items[4].id == MI_SETTINGS &&
          game.menu.items[5].id == MI_QUIT, "title: PRACTICE / 2 PLAYER / TOURNAMENT / MUSIC / SETTINGS / QUIT");
    frames(1);
    CHECK(input.context == INPUT_MENU, "menus use INPUT_MENU");
    event(INPUT_NAV_DOWN, 0); event(INPUT_NAV_DOWN, 0); event(INPUT_NAV_DOWN, 0); event(INPUT_NAV_DOWN, 0); event(INPUT_NAV_DOWN, 0);
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
    CHECK(n > 12 && memcmp(buf, "BYCS", 4) == 0 && buf[4] == 0 && buf[5] == 4, "magic + big-endian version 4");
    CHECK(save_decode(&d, buf, n) && d.profile[0].full_speed == 3.1 && d.profile[1].full_speed == 2.45,
          "version 4 preserves custom SET POWER and factory speed references");
    CHECK(save_decode(&d, buf, n) && d.career_fight == 5 && d.career_wins == 7 && d.profile[1].left_handed &&
          d.profile[0].full_speed == 3.1 && d.profile[1].spin_strength == 0.4 && d.help_tucked && d.wind_on, "round trip");
    buf[8] ^= 1;
    CHECK(!save_decode(&d, buf, n) && d.career_fight == 0 && !d.wind_on, "CRC failure falls back to defaults");
    s.career_fight = 6; s.career_wins = 2; n = save_encode(&s, buf, sizeof buf);
    CHECK(save_decode(&d, buf, n) && d.career_fight == 0 && d.career_wins == 0 && d.profile[1].left_handed, "invalid career field defaults alone");
    s.profile[1].full_speed = swing_speed(1.2);
    n = save_encode(&s, buf, sizeof buf);
    CHECK(save_decode(&d, buf, n) && d.profile[1].full_speed == s.profile[1].full_speed,
          "current saves keep a custom speed equal to the old default");
    buf[5] = 2;   /* version 2 has the same field layout */
    uint32_t crc = save_crc32(buf, n - 4);
    for (int i = 0; i < 4; i++) buf[n - 4 + i] = (uint8_t)(crc >> (24 - i * 8));
    CHECK(save_decode(&d, buf, n) && d.profile[0].full_speed == 3.1 &&
          d.profile[1].full_speed == 2.45 &&
          d.profile[1].left_handed && d.profile[1].spin_strength == .4,
          "older saves update the factory speed and keep custom speed, hand and curl");
    s.profile[1].full_speed=swing_speed(1.9);
    n=save_encode(&s,buf,sizeof buf);
    CHECK(save_decode(&d,buf,n) && d.profile[1].full_speed==s.profile[1].full_speed,
          "version 4 keeps a custom speed equal to the 0.1.7 factory speed");
    buf[5]=3;
    crc=save_crc32(buf,n-4);
    for (int i=0;i<4;i++) buf[n-4+i]=(uint8_t)(crc>>(24-i*8));
    CHECK(save_decode(&d,buf,n) && d.profile[0].full_speed==3.1 &&
          d.profile[1].full_speed==2.45 && d.profile[1].left_handed,
          "version 3 migrates the 0.1.7 factory speed and keeps custom power and hand");
    CHECK(save_crc32((const uint8_t *)"123456789", 9) == 0xCBF43926u, "CRC-32 check value");
}

/* ---------------- review fixes: WG-1 .. WG-5 ---------------- */
static void practice_ready(uint32_t seed) {
    boot(seed, NULL); game_menu_select(&game, MI_OK);
    game_start(&game, true, false, false);
    while (game.phase != PH_AIM) frames(1);
    frames(1);
}

static void test_last_shot_gauge(void) {
    practice_ready(113);
    CHECK(!game.last_shot, "no LAST SHOT until a bag launches");
    CHECK(game_throw(&game, SWING_POWER_CAP, 0, 0), "cap throw launches");
    CHECK(game.phase == PH_FLYING && game.last_shot && game_last_shot_percent(&game) == 100,
          "LAST SHOT shows 100%% in flight at the cap");
    CHECK(!game_throw(&game, .25, 0, 0) && game_last_shot_percent(&game) == 100,
          "a rejected throw leaves LAST SHOT in place");
    frames(300);
    CHECK(game.phase == PH_AIM && game.practice_throws == 1 && !game.arrow_on &&
          game.last_shot && game.last_shot_power == SWING_POWER_CAP && game_last_shot_percent(&game) == 100,
          "LAST SHOT survives scoring, the next aim phase and the arrow fading");
    CHECK(game_throw(&game, .40, 0, 0) && game.last_shot && game.last_shot_power == .40 &&
          game_last_shot_percent(&game) == 53, "the next launch replaces LAST SHOT with its rounded percent");
    frames(300);
    game.arc = ARC_KOBE;
    double lob_power = physical_power(game.win, 0, game.side[0], ARC_KOBE, SWING_POWER_CAP);
    CHECK(game_throw(&game, SWING_POWER_CAP, 0, 0) && game.last_shot_power == lob_power &&
          game_last_shot_percent(&game) == 100, "LAST SHOT uses remapped KOBE launch power and clamps the label");
}

static void test_deferred_release_stance(void) {
    practice_ready(109);
    InputSlot *slot = &input.slots[0];
    slot->stance_from = 0;
    slot->stance = SWING_STEP;
    slot->stance_ms = now_ms;
    uint64_t release_ms = now_ms + SWING_STEP_EASE_MS / 2;
    frames(8);
    CHECK(game.position[0] == SWING_STEP, "stance finishes easing before release confirmation");
    InputEvent e = {.action=INPUT_THROW,
        .toss={.thrown=true, .power=0.5, .release=SWING_B_UP, .release_ms=release_ms}};
    game_input(&game, &e);
    CHECK(game.nbags == 1 && fabs(game.arrow_x - SWING_STEP / 2) < 1e-9,
          "deferred launch uses the stance at B-up (%.3f)", game.arrow_x);
}
static void test_clear_board_any_phase(void) {
    /* WG-1: CLEAR BOARD while the bag is in the air must leave a playable practice. */
    practice_ready(101);
    CHECK(game_throw(&game, 0.66, 0, 0), "throw");
    frames(120); while (game.phase != PH_AIM) frames(1);
    int throws = game.practice_throws, in = game.practice_in, on = game.practice_on;
    CHECK(game_throw(&game, 0.66, 0, 0), "second throw");
    frames(10);
    CHECK(game.phase == PH_FLYING && game.active >= 0, "bag is in flight");
    event(INPUT_PAUSE, 0);
    CHECK(game.ui == UI_PAUSE && menu_index(MI_CLEAR_BOARD) == 1, "pause offers CLEAR BOARD during flight");
    game_menu_select(&game, MI_CLEAR_BOARD);
    CHECK(game.nbags == 0 && game.active == -1, "board cleared mid-flight");
    CHECK(game.phase == PH_AIM && game.ui == UI_NONE && !game.paused, "practice is back at AIM after clearing mid-flight (phase %d)", game.phase);
    CHECK(game.practice_throws == throws && game.practice_in == in && game.practice_on == on, "stats kept; the cleared in-flight bag is not counted");
    CHECK(!game.arrow_on, "the cleared bag's arrow is gone");
    feed_still(0, false); frames(2);
    CHECK(input.context == INPUT_PLAY && !input.paused, "input is live again");
    CHECK(game_throw(&game, 0.66, 0, 0), "another throw is accepted after clearing mid-flight");
    frames(600);
    CHECK(game.phase == PH_AIM && game.practice_throws == throws + 1, "that throw lands and is counted (%d)", game.practice_throws);
    /* while waiting between bags (a KNOCKED IN!/next-turn timer pending) */
    CHECK(game_throw(&game, 0.66, 0, 0), "throw");
    while (game.phase == PH_FLYING) frames(1);
    CHECK(game.phase == PH_WAIT, "between bags");
    game_clear_board(&game);
    CHECK(game.phase == PH_AIM && game.nbags == 0, "clearing between bags hands over a fresh bag at once");
    frames(200);
    CHECK(game.phase == PH_AIM && game.nbags == 0, "no stale next-turn timer fires afterwards");
    /* while aiming */
    game_clear_board(&game);
    CHECK(game.phase == PH_AIM && game_throw(&game, 0.66, 0, 0), "clearing while aiming keeps play going");
    /* not in a match */
    game_start(&game, false, false, false); while (game.phase != PH_AIM) frames(1);
    game_throw(&game, 0.66, 0, 0); frames(5);
    game_clear_board(&game);
    CHECK(game.nbags == 1 && game.phase == PH_FLYING, "CLEAR BOARD does nothing outside practice");
}

static void finish_round(void) {
    int guard = 0;
    while (game.ui != UI_SUMMARY && guard++ < 60 * 600) {
        if (game.phase == PH_AIM && game.ui == UI_NONE && game_is_human(&game, game_thrower(&game))) game_throw(&game, 0.70, 0, 0);
        frames(1);
    }
}
static void test_pause_on_summary(void) {
    /* WG-2: HOME on the round summary, then RESUME, must bring NEXT ROUND back. */
    boot(111, NULL); game_menu_select(&game, MI_OK);
    game_start(&game, false, false, false);
    finish_round();
    CHECK(game.ui == UI_SUMMARY && game.phase == PH_SUMMARY, "round summary after eight bags");
    int inning = game.inning;
    event(INPUT_PAUSE, 0);
    CHECK(game.ui == UI_PAUSE && game.paused, "HOME pauses on the summary");
    event(INPUT_CONFIRM, 0);   /* RESUME */
    CHECK(game.ui == UI_SUMMARY && !game.paused && menu_index(MI_NEXT) == 0, "RESUME returns to the summary with NEXT ROUND");
    CHECK(!strcmp(game.menu.items[0].label, "NEXT ROUND"), "button reads NEXT ROUND");
    event(INPUT_PAUSE, 0); event(INPUT_PAUSE, 0);
    CHECK(game.ui == UI_SUMMARY, "HOME twice (pause, resume) also returns to the summary");
    event(INPUT_PAUSE, 0); game_menu_select(&game, MI_SETTINGS); event(INPUT_BACK, 0);
    CHECK(game.ui == UI_PAUSE, "SETTINGS from that pause comes back to PAUSED");
    event(INPUT_BACK, 0);
    CHECK(game.ui == UI_SUMMARY, "B on PAUSED returns to the summary");
    feed_still(0, false); frames(2);
    event(INPUT_CONFIRM, 0);
    CHECK(game.ui == UI_NONE && game.inning == inning + 1, "A starts the next round");
    frames(120);
    CHECK(game.phase == PH_AIM && !input.paused && input.context == INPUT_PLAY, "next round is playable (phase %d)", game.phase);
    CHECK(game_throw(&game, 0.7, 0, 0), "throw accepted in the next round");
    /* FINAL screen */
    frames(600); while (game.phase != PH_AIM && game.ui == UI_NONE) frames(1);
    game.scores[0] = 20;
    int f = play_match(0.70, 60 * 60 * 40);
    CHECK(f >= 0 && game.phase == PH_OVER && game.ui == UI_SUMMARY, "match reaches FINAL");
    event(INPUT_PAUSE, 0);
    CHECK(game.ui == UI_PAUSE, "HOME pauses on FINAL");
    event(INPUT_CONFIRM, 0);
    CHECK(game.ui == UI_SUMMARY && !strcmp(game.menu.items[0].label, "NEW MATCH"), "RESUME returns to FINAL with NEW MATCH");
    event(INPUT_CONFIRM, 0);
    CHECK(game.ui == UI_TITLE && game.phase == PH_MENU, "NEW MATCH goes back to the menu");
    /* MAIN MENU from a paused summary */
    game_start(&game, false, false, false); finish_round();
    event(INPUT_PAUSE, 0); game_menu_select(&game, MI_MAIN_MENU);
    CHECK(game.ui == UI_TITLE && game.phase == PH_MENU && game.pause_return == UI_NONE, "MAIN MENU from a paused summary");
    game_start(&game, true, false, false); while (game.phase != PH_AIM) frames(1);
    event(INPUT_PAUSE, 0); event(INPUT_CONFIRM, 0);
    CHECK(game.ui == UI_NONE, "a later ordinary pause resumes to play, not to an old summary");
    /* tournament result */
    boot(113, NULL); game_menu_select(&game, MI_OK);
    game_menu_select(&game, MI_TOURNAMENT); game_menu_select(&game, MI_CONTINUE); game_menu_select(&game, MI_LETS_TOSS);
    game.cpu_step_budget = 2000000;
    game.scores[1] = 20;
    int guard = 0;
    while (game.ui != UI_CAREER_RESULT && guard++ < 60 * 900) {
        if (game.ui == UI_SUMMARY) game_menu_select(&game, MI_NEXT);
        if (game.phase == PH_AIM && game.ui == UI_NONE && game_is_human(&game, 0) && game_thrower(&game) == 0) game_throw(&game, 0.2, 0.1, 0);
        frames(1);
    }
    CHECK(game.ui == UI_CAREER_RESULT, "tournament result screen");
    event(INPUT_PAUSE, 0);
    CHECK(game.ui == UI_PAUSE, "HOME pauses on the tournament result");
    event(INPUT_CONFIRM, 0);
    CHECK(game.ui == UI_CAREER_RESULT && menu_index(MI_NEXT) == 0 && menu_index(MI_CIRCUIT_MAP) == 1, "RESUME returns to the result with TRY AGAIN / CIRCUIT MAP");
    /* HOME where no pause menu exists must not leave the input layer paused */
    boot(115, NULL); game_menu_select(&game, MI_OK);
    InputSample home; memset(&home, 0, sizeof home);
    home.motion.ms = now_ms; home.motion.g = (SwingVector){0, 0, 1}; home.motion.accel_valid = true; home.buttons_valid = true; home.buttons = INPUT_HOME;
    input_feed(&input, 0, &home); drain();
    CHECK(game.ui == UI_TITLE, "HOME on the title stays on the title");
    feed_still(0, false); frames(2);
    CHECK(!input.paused, "input layer released after HOME on the title");
}

static void test_unlimited_practice(void) {
    /* WG-5: no cap and no forced CLEAR BOARD; every visible bag is kept. */
    practice_ready(121);
    const int total = 1500;
    int throws = 0, guard = 0, refused = 0, max_cap = 0, holed = 0, lost = 0, compactions = 0, last_banked = 0;
    while (throws < total && guard++ < total * 900) {
        if (game.phase == PH_AIM) {
            CHECK(game.ui == UI_NONE, "practice never opens a forced panel (throw %d, ui %d)", throws, game.ui);
            /* mostly board shots (they stay), with grounders and long misses (they are removed) mixed in */
            double p = throws % 8 == 3 ? 0.25 : throws % 8 == 6 ? 0.95 : 0.66 + (rng_next(&game.rng) - .5) * 0.08;
            if (game_throw(&game, p, (rng_next(&game.rng) - .5) * 0.05, 0)) throws++; else refused++;
        }
        if (game.bag_cap > max_cap) max_cap = game.bag_cap;
        frames(1);
        /* holed bags (thrown in or knocked in) must never be lost when the list is compacted */
        int in_now = game.banked.in_c[0];
        for (int i = 0; i < game.nbags; i++) in_now += game.bags[i].state == BAG_IN;
        if (in_now < holed) lost++;
        holed = in_now;
        if (game.banked.in_c[0] != last_banked) { compactions++; last_banked = game.banked.in_c[0]; }
    }
    while (game.phase != PH_AIM && guard++ < total * 900) frames(1);
    CHECK(throws == total && refused == 0, "%d practice throws, none refused (%d thrown, %d refused)", total, throws, refused);
    CHECK(game.practice_throws == total, "all throws counted (%d)", game.practice_throws);
    int visible = 0, in_list = 0, on_board = 0;
    for (int i = 0; i < game.nbags; i++) {
        BagState st = game.bags[i].state;
        in_list += st == BAG_IN;
        visible += st != BAG_IN && st != BAG_REMOVED;
        on_board += st == BAG_BOARD && !game.bags[i].dead;
    }
    CHECK(visible > 256, "more than 256 bags are still lying in the yard (%d)", visible);
    CHECK(game.nbags > 256 && game.bag_cap >= game.nbags, "storage grew past the old 256 cap (%d bags, cap %d)", game.nbags, game.bag_cap);
    CHECK(compactions > 0 && lost == 0, "holed bags were compacted out of the list %d times and none was lost", compactions);
    CHECK(holed >= game.practice_in && holed == game.banked.in_c[0] + in_list, "holed total covers thrown-in and knocked-in bags (%d >= %d)", holed, game.practice_in);
    CHECK(game.live.in_c[0] == holed && game.live.on_c[0] == on_board && game.live.pts[0] == 3 * holed + on_board,
          "practice POINTS readout = 3 x holed + on the board, unchanged by compaction (%d)", game.live.pts[0]);
    printf("  practice: %d throws, %d bags kept (%d visible), cap %d\n", total, game.nbags, visible, max_cap);
    event(INPUT_PAUSE, 0); game_menu_select(&game, MI_CLEAR_BOARD);
    CHECK(game.nbags == 0 && game.live.pts[0] == 0 && game.banked.pts[0] == 0 && game.practice_throws == total, "CLEAR BOARD still clears everything and keeps stats");
    CHECK(game.phase == PH_AIM && game_throw(&game, 0.66, 0, 0), "and practice continues");
    /* compaction keeps order and never drops a visible bag */
    practice_ready(123);
    game.bag_cap_limit = game.bag_cap;   /* simulate: no more memory */
    int kept = 0; throws = 0; guard = 0;
    while (throws < 400 && guard++ < 400 * 900) {
        if (game.phase == PH_AIM) {
            int before_visible = 0;
            for (int i = 0; i < game.nbags; i++) before_visible += game.bags[i].state != BAG_IN && game.bags[i].state != BAG_REMOVED;
            if (!game_throw(&game, 0.25, 0, 0)) break;   /* grounders: removed after 0.7 s */
            throws++;
            int after_visible = 0;
            for (int i = 0; i < game.nbags - 1; i++) after_visible += game.bags[i].state != BAG_IN && game.bags[i].state != BAG_REMOVED;
            kept += after_visible == before_visible;
        }
        frames(1);
    }
    CHECK(throws == 400 && kept == 400, "with memory capped, 400 grounders still play: gone bags are reused, visible ones never dropped (%d/%d)", kept, throws);
    CHECK(game.nbags <= game.bag_cap_limit, "stayed inside the fixed block (%d)", game.nbags);
}

/* Physical reports through the real input and game update routes. */
typedef struct { bool live_seen, thrown; double live_yaw, live_power, live_x, yaw, power, bag_yaw, back_live; } SwingSeen;

static SwingSeen drive_bowling(const BowlingTrace *trace, double release_pitch, uint32_t noise, Bag *launch) {
    SwingSeen seen={0};
    int bags=game.nbags;
    double release=bowling_release(trace);
    for (unsigned tick=0;tick<=3000;tick+=10) {
        double seconds=tick/1000.0;
        bool held=bowling_button(trace,seconds);
        SwingVector g=bowling_force(trace,seconds);
        if (isfinite(release_pitch) && seconds>=release && seconds<release+.01) {
            double pitch=release_pitch*SWING_PI/180;
            g=(SwingVector){0,-sin(pitch),cos(pitch)};
        }
        uint32_t buttons=(held ? INPUT_B : 0) | ((tick/10)%2 ? noise : 0);
        InputSample report={.motion={.ms=(now_ms+=10),.g=g,.accel_valid=true},
            .buttons=buttons,.buttons_valid=true};
        input_feed(&input,0,&report); drain();
        if (game.nbags==bags+1) {
            const Bag *b=&game.bags[game.nbags-1];
            if (launch) *launch=*b;
            seen.thrown=true;
            seen.yaw=game.arrow_yaw; seen.power=game.arrow_power; seen.bag_yaw=atan2(b->v.x,-b->v.z);
            break;
        }
        game_update(&game,.01,now_ms); drain();
        if (seconds<bowling_forward(trace)) seen.back_live+=game.arrow_live;
        if (game.arrow_on && game.arrow_live) {
            seen.live_seen=true; seen.live_yaw=game.arrow_yaw;
            seen.live_power=game.arrow_power; seen.live_x=game.arrow_x;
        }
    }
    return seen;
}

static SwingSeen bowling_swing(double forward_s, double lateral, bool release) {
    BowlingTrace trace=bowling_trace();
    trace.forward_s=forward_s;
    trace.lateral_g=-lateral; /* Raw +X points left, positive game yaw points right. */
    trace.release_deg=40;
    trace.held=!release;
    return drive_bowling(&trace,NAN,0,NULL);
}
static void next_bag(void) { int guard = 0; while (game.phase != PH_AIM && guard++ < 6000) frames(1); frames(1); }
/* ---------------- Swing speed and direction, picker-only height ---------------- */
static void remote_feed_channel(unsigned ch, double e, double pitch, double roll, uint32_t buttons) {
    now_ms += 10;
    double p = pitch * SWING_PI / 180, r = roll * SWING_PI / 180;
    InputSample s; memset(&s, 0, sizeof s);
    s.motion.ms = now_ms; s.motion.g = (SwingVector){(1 + e) * sin(r) * cos(p), -(1 + e) * sin(p), (1 + e) * cos(r) * cos(p)};
    s.motion.accel_valid = true; s.motion.b = (buttons & INPUT_B) != 0; s.buttons_valid = true; s.buttons = buttons;
    input_feed(&input, ch, &s); drain();
}
static void remote_feed(double e, double pitch, double roll, uint32_t buttons) {
    remote_feed_channel(0, e, pitch, roll, buttons);
}
static void remote_buttons(double e, double pitch, double roll, uint32_t buttons) {
    remote_feed(e, pitch, roll, buttons);
    game_update(&game, 0.01, now_ms); drain();
}
static void tap(uint32_t button) { remote_buttons(0, 0, 0, button); remote_buttons(0, 0, 0, 0); }

static void test_practice_boot_ignores_stale_b(void) {
    const struct {
        const char *name;
        uint32_t confirm;
        bool menu_b, first_play_b, unknown_b;
    } cases[] = {
        {"B held across A and into play", INPUT_A, true, true, false},
        {"B-up on the first play report after 2", INPUT_TWO, true, false, false},
        {"first report from the active remote is B held", INPUT_TWO, false, true, true},
        {"accelerometer motion with B up", INPUT_A, false, false, false},
    };
    for (unsigned k = 0; k < sizeof cases / sizeof cases[0]; k++) {
        boot(140, NULL);
        CHECK(game.ui == UI_STRAP, "%s: starts at the strap screen", cases[k].name);
        unsigned menu_ch = cases[k].unknown_b ? 1 : 0;
        if (menu_ch == 1) input_connection(&input, menu_ch, true);
        uint32_t held = cases[k].menu_b ? INPUT_B : 0;
        remote_feed_channel(menu_ch, 0, 0, 0, cases[k].confirm | held);
        CHECK(game.ui == UI_TITLE, "%s: confirms strap", cases[k].name);
        remote_feed_channel(menu_ch, 0, 0, 0, held);
        remote_feed_channel(menu_ch, .7, -30, 0, cases[k].confirm | held);
        frames(60);
        CHECK(game.practice && game.phase == PH_AIM && input.context == INPUT_PLAY,
              "%s: enters Practice through the title", cases[k].name);

        if (cases[k].first_play_b) remote_buttons(.8, 0, 0, INPUT_B);
        CHECK(!swing_armed(&input.detector), "%s: no grip from a stale B", cases[k].name);
        remote_buttons(.8, 25, 0, 0);
        CHECK(game.nbags == 0 && game.phase == PH_AIM, "%s: no bag on B-up", cases[k].name);
        for (int n = 0; n < 20; n++) remote_buttons(1.5, n % 2 ? 25 : -35, 12, 0);
        frames(300);
        CHECK(game.nbags == 0 && game.phase == PH_AIM && game.practice_throws == 0,
              "%s: no phantom throw after motion or B-up (throws %d)", cases[k].name, game.practice_throws);

        remote_buttons(.25, -5, 0, INPUT_B);
        CHECK(swing_armed(&input.detector), "%s: a fresh moving B-down grips at once", cases[k].name);
        remote_buttons(.25, -5, 0, 0);
        CHECK(game.practice_throws==0 && game.nbags==0, "%s: a grip without a swing keeps the bag", cases[k].name);
        SwingSeen fresh=bowling_swing(.45,0,true);
        CHECK(fresh.thrown,"%s: deliberate physical swing throws",cases[k].name);
        CHECK(game.phase == PH_FLYING && game.nbags == 1,
              "%s: a fresh bowling swing launches one bag", cases[k].name);
        frames(300);
        CHECK(game.practice_throws == 1, "%s: the deliberate throw is counted once", cases[k].name);
    }
}
static void test_practice_cancel_and_retry(void) {
    practice_ready(143);
    for (int gesture=0;gesture<3;gesture++) {
        BowlingTrace trace=bowling_trace();
        trace.still=gesture==0;
        trace.release_going_back=gesture==1;
        trace.release_deg=-20;
        SwingSeen rejected=drive_bowling(&trace,NAN,0,NULL);
        SwingReject reason=gesture==0 ? SWING_REJECT_NO_SWING :
            gesture==1 ? SWING_REJECT_BACKSWING : SWING_REJECT_BEFORE_BOTTOM;
        CHECK(!rejected.thrown && input.detector.last_reject==reason,
              "gesture %d rejected with reason %d",gesture,input.detector.last_reject);
        CHECK(input.detector.phase==SWING_READY,"gesture %d returns to READY",gesture);
        CHECK(game.phase==PH_AIM && game.nbags==0 && game.practice_throws==0,
              "gesture %d keeps the practice bag and the throw count",gesture);
    }
    SwingSeen toss=bowling_swing(.45,0,true);
    CHECK(toss.thrown && game.phase == PH_FLYING, "the next forward swing launches on B-up");
    frames(300);
    CHECK(game.practice_throws == 1, "only the forward swing counts");
}
/* The throw-type picker through the real input layer: D-pad DOWN twice (SLIDER), then UP h times. */
static void pick_height(int h) {
    tap(INPUT_DOWN); tap(INPUT_DOWN);
    for (int i = 0; i < h; i++) tap(INPUT_UP);
}
#define LAND_MAX 600
typedef struct { bool thrown, deck; double land_x, land_z, peak_y, power, aim, preview_power; int n; double y[LAND_MAX], x[LAND_MAX], z[LAND_MAX]; } Landing;
/* Step the launched bag from release at the game's fixed 240 Hz until touchdown. */
static Landing swing_and_land(double forward_s, double lateral, double release_pitch, uint32_t noise) {
    static Landing l; memset(&l,0,sizeof l);
    next_bag();
    game_clear_board(&game); frames(1);
    BowlingTrace trace=bowling_trace();
    trace.forward_s=forward_s;
    trace.lateral_g=-lateral;
    trace.release_s=bowling_forward(&trace)+forward_s+.06;
    Bag launch={0};
    SwingSeen seen=drive_bowling(&trace,release_pitch,noise,&launch);
    l.thrown=seen.thrown;
    if (!l.thrown) return l;
    l.preview_power=seen.live_power;
    l.power = input.stats.last_toss.power; l.aim = input.stats.last_toss.aim;
    Wind calm = game.wind;
    while (launch.state == BAG_FLY && l.n < LAND_MAX) {
        l.x[l.n] = launch.p.x; l.y[l.n] = launch.p.y; l.z[l.n] = launch.p.z; l.n++;
        l.peak_y = fmax(l.peak_y, launch.p.y);
        physics_substep(&launch, 1, &calm);
    }
    l.land_x = launch.p.x; l.land_z = launch.p.z;
    l.deck = bag_on_deck(&launch) || launch.state == BAG_HOLE;
    frames(300);
    return l;
}
static bool same_flight(const Landing *a, const Landing *b, bool vertical_only) {
    if (a->n != b->n) return false;
    for (int i = 0; i < a->n; i++) {
        if (fabs(a->y[i] - b->y[i]) > 1e-12) return false;
        if (!vertical_only && (fabs(a->x[i] - b->x[i]) > 1e-12 || fabs(a->z[i] - b->z[i]) > 1e-12)) return false;
    }
    return true;
}
static void test_set_power_reaches_hole(void) {
    boot(142, NULL);
    tap(INPUT_A);
    game_menu_select(&game, MI_SETTINGS);
    game_menu_select(&game, MI_SETPOWER_RED);
    BowlingTrace easy=bowling_trace();
    easy.forward_s=.65;
    easy.release_s=bowling_forward(&easy)+easy.forward_s+.06;
    for (int toss=0;toss<3;toss++) drive_bowling(&easy,NAN,0,NULL);
    double calibrated_speed=input.stats.last_toss.speed;
    SwingCalibrationLimits limits = game_calibration_limits(&game);
    CHECK(game.setpower_count == 3 && !input.calibrating && game.save_dirty && game.nbags == 0,
          "SET POWER saves three easy tosses without launching a bag");
    CHECK(fabs(swing_power(calibrated_speed, game.save.profile[0].full_speed * swing_type_scale(1)) - limits.target_power) < 1e-9,
          "the saved easy toss power matches the hole target");
    CHECK(!input_begin_calibration(&input, (SwingCalibrationLimits){.8, limits.full_speed_floor, limits.clip_ceiling}),
          "SET POWER rejects an unreachable target above the cap");

    game_menu_select(&game, MI_OK);
    game_menu_select(&game, MI_BACK);
    tap(INPUT_A);
    Landing l = swing_and_land(.65, 0, NAN, 0);
    CHECK(l.thrown && fabs(l.power - limits.target_power) < 1e-9 && game.practice_in == 1,
          "a calibrated easy toss scores a NORMAL/SLICK cornhole in Practice (power %.3f, in %d)",
          l.power, game.practice_in);
}
static void test_swing_to_landing(void) {
    practice_ready(141);
    CHECK(game.wind.mph == 0, "practice is calm (%g mph)", game.wind.mph);
    const double durations[]={.75,.65,.55,.50,.46};
    const char *names[]={"SLIDER","NORMAL","KOBE"};
    const double front=BYC_REL_Z-BYC_BOARD_Z, back=front+BYC_L*cos(BYC_TH);
    printf("  deck spans %.4f–%.4f m from release\n",front,back);
    for (int h=0;h<3;h++) {
        next_bag(); pick_height(h);
        CHECK(game.arc==(BagArc)h && game.heights[0]==h,"picker on %s",names[h]);
        double previous_distance=0,previous_power=0;
        for (unsigned k=0;k<sizeof durations/sizeof durations[0];k++) {
            double duration=durations[k];
            Landing l=swing_and_land(duration,0,NAN,0);
            double distance=BYC_REL_Z-l.land_z;
            double expected=swing_power(input.stats.last_toss.speed,
                                        SWING_DEFAULT_FULL_SPEED*swing_type_scale(h));
            printf("  %s %.2f s: %.6f power -> %.6f m (%s)\n",
                   names[h],duration,l.power,distance,l.deck ? "deck" : "ground");
            CHECK(l.thrown && l.n>20,"%s %.2f s throws and flies (%d steps)",names[h],duration,l.n);
            CHECK(fabs(l.land_x)<1e-9,"a straight swing lands on the centre line (x=%g)",l.land_x);
            CHECK(fabs(l.power-expected)<1e-9,"%s %.2f s gives %.3f power (got %.3f)",names[h],duration,expected,l.power);
            CHECK(fabs(l.preview_power-physical_power(game.win,0,SIDE_SLICK,(BagArc)h,expected))<1e-9,
                  "%s %.2f s preview matches release power",names[h],duration);
            if (previous_power<SWING_POWER_CAP)
                CHECK(previous_power<l.power && previous_distance<distance,
                      "%s %.2f s lands farther (%.2f m)",names[h],duration,distance);
            else
                CHECK(l.power==SWING_POWER_CAP && distance==previous_distance,
                      "%s %.2f s stays at the cap (%.2f m)",names[h],duration,distance);
            previous_power=l.power;
            previous_distance=distance;
        }
    }
    next_bag(); pick_height(1);
    /* The same direction of motion aims the same way for either hand. */
    Landing r = swing_and_land(.65, .55, NAN, 0), lft = swing_and_land(.65, -.55, NAN, 0);
    printf("  swing right -> x %+.3f m, swing left -> x %+.3f m\n", r.land_x, lft.land_x);
    CHECK(r.thrown && lft.thrown && r.land_x > 0.1 && lft.land_x < -0.1, "right swing lands right, left swing lands left (%g, %g)", r.land_x, lft.land_x);
    CHECK(fabs(r.land_x + lft.land_x) < 1e-9, "mirror images");
    SwingProfile lefty = input.slots[0].profile; lefty.left_handed = true; input_set_profile(&input, 0, lefty);
    Landing lr = swing_and_land(.65, .55, NAN, 0), ll = swing_and_land(.65, -.55, NAN, 0);
    CHECK(lr.land_x>.1 && ll.land_x<-.1 && fabs(lr.land_x-r.land_x)<1e-9, "LEFT HAND uses the same world-space aim (%g, %g)", lr.land_x, ll.land_x);
    lefty.left_handed = false; input_set_profile(&input, 0, lefty);
    /* -/+ pressed throughout the swing change nothing */
    Landing base = swing_and_land(.65, .55, NAN, 0), plus = swing_and_land(.65, .55, NAN, INPUT_PLUS), minus = swing_and_land(.65, .55, NAN, INPUT_MINUS);
    CHECK(plus.thrown && minus.thrown && same_flight(&base, &plus, false) && same_flight(&base, &minus, false), "-/+ during play leave the flight identical");
    /* height: release pitch never changes the arc; only the picker does */
    const double pitches[4] = {-30, 0, 25, 80};
    Landing straight[4], lean[4];
    for (int k = 0; k < 4; k++) { straight[k] = swing_and_land(.65, 0, pitches[k], 0); }
    for (int k = 0; k < 4; k++) { lean[k] = swing_and_land(.65, .55, pitches[k], 0); }
    bool same = true, same_lean = true;
    for (int k = 1; k < 4; k++) {
        same = same && straight[k].thrown && same_flight(&straight[0], &straight[k], false);
        same_lean = same_lean && lean[k].thrown && same_flight(&lean[0], &lean[k], true) && fabs(lean[k].peak_y - lean[0].peak_y) < 1e-12;
    }
    CHECK(same, "release pitch -30/0/25/80 deg: identical flight");
    CHECK(same_lean, "with a sideways swing too: identical arc (height profile)");
    Landing arc[3];
    for (int h = 0; h < 3; h++) {
        next_bag(); pick_height(h);
        CHECK(game.heights[0] == h, "picker set to %d", h);
        arc[h] = swing_and_land(.65, 0, NAN, 0);
        next_bag();
        CHECK(game.arc == (BagArc)h, "picker stays on %d for the next bag", h);
    }
    printf("  SLIDER / NORMAL / KOBE peak height %.2f / %.2f / %.2f m\n", arc[0].peak_y, arc[1].peak_y, arc[2].peak_y);
    CHECK(arc[0].peak_y + 0.1 < arc[1].peak_y && arc[1].peak_y + 0.1 < arc[2].peak_y, "slider < normal < lob: three distinct trajectories");
    CHECK(!same_flight(&arc[0], &arc[1], true) && !same_flight(&arc[1], &arc[2], true), "and the flights differ");
    /* D-pad left/right still steps the stance */
    next_bag(); pick_height(1);
    double before = input.slots[0].stance;
    tap(INPUT_RIGHT); frames(10);
    CHECK(fabs(input.slots[0].stance - before - SWING_STEP) < 1e-9 && fabs(game.position[0] - input.slots[0].stance) < 1e-9, "D-pad RIGHT steps the stance (%g)", game.position[0]);
    tap(INPUT_LEFT); tap(INPUT_LEFT); frames(10);
    CHECK(fabs(game.position[0] - (before - SWING_STEP)) < 1e-9, "D-pad LEFT steps back (%g)", game.position[0]);
    tap(INPUT_TWO); frames(10);
}
static void test_version_tag(void) {
    CHECK(strcmp(BYC_VERSION, "0.1.9") == 0, "version is 0.1.9 (%s)", BYC_VERSION);
    CHECK(strcmp(game_version_tag(), "v0.1.9") == 0, "version tag v0.1.9 (%s)", game_version_tag());
}
/* 0.1.3: at 16:9 the 2D layer is 853.3 x 480 square units and the BOARD inset sits clear of the
 * beer-pong table and both players; 4:3 keeps the 0.1.2 layout. */
static void test_widescreen_layout(void) {
    CHECK(fabs(view_ui_width(4.0 / 3) - 640) < 1e-9 && fabs(view_ui_width(16.0 / 9) - 480 * 16.0 / 9) < 1e-9, "2D width 640 at 4:3, 853.3 at 16:9");
    int x, y, w, h;
    view_board_inset(4.0 / 3, &x, &y, &w, &h);
    CHECK(x == 28 && y == 92 && w == 132 && h == 150, "4:3 inset unchanged");
    view_board_inset(16.0 / 9, &x, &y, &w, &h);
    double fx0 = x - 3, fy0 = y - 22, fx1 = x + w + 3 + 5, fy1 = y + h + 3 + 5;   /* frame + drop shadow */
    CHECK(fx0 >= 0 && fy0 >= 0, "16:9 inset inside the screen");
    /* boxes: the table, and each pong player (0.6 m square, 1.85 m tall) at their table-local spots */
    const double box[3][4] = {{-1.06, 1.06, -.44, PARTY_TABLE_TOP}, {-1.59, -.99, -.22, 1.85}, {.99, 1.59, .12, 1.85}};
    for (int b = 0; b < 3; b++) {
        double sx0 = 1e9, sx1 = -1e9, sy0 = 1e9, sy1 = -1e9;
        for (int i = 0; i < 2; i++) for (int j = -1; j <= 1; j += 2) for (int k = 0; k < 2; k++) {
            double lz = b == 0 ? j * .44 : box[b][2] + j * .3;
            Vec3 wp = party_table_to_world((Vec3){box[b][i], k ? box[b][3] : 0, lz});
            double px, py;
            if (!view_project(wp, 16.0 / 9, &px, &py)) continue;
            sx0 = fmin(sx0, px); sx1 = fmax(sx1, px); sy0 = fmin(sy0, py); sy1 = fmax(sy1, py);
        }
        bool overlap = sx0 < fx1 && sx1 > fx0 && sy0 < fy1 && sy1 > fy0;
        CHECK(!overlap, "16:9 inset (%.0f..%.0f, %.0f..%.0f) clears %s (%.0f..%.0f, %.0f..%.0f)", fx0, fx1, fy0, fy1,
              b == 0 ? "the pong table" : b == 1 ? "the near pong player" : "the far pong player", sx0, sx1, sy0, sy1);
    }
    /* the centred scoreboard (360 wide at y 14) and the inset don't touch */
    CHECK(fx1 < view_ui_width(16.0 / 9) / 2 - 180, "inset left of the scoreboard");
}
static void test_combined_arrow(void) {
    /* WG-3: one arrow, live during the forward swing, direction = launch, length = power. */
    practice_ready(131);
    SwingSeen straight = bowling_swing(.65, 0, true);
    CHECK(straight.back_live == 0, "no arrow during the backswing or the top-of-backswing pause");
    CHECK(straight.live_seen, "arrow appears during the forward swing, before release");
    CHECK(straight.thrown, "letting go of B throws");
    CHECK(fabs(straight.live_yaw) < 1e-12 && fabs(straight.yaw) < 1e-12 && fabs(straight.bag_yaw) < 1e-12, "straight swing: arrow and bag go straight (%g)", straight.yaw);
    CHECK(game.arrow_on && !game.arrow_live, "arrow stays on after release");
    CHECK(fabs(straight.live_power - straight.power) < 1e-9, "the preview shows the power the release throws (%g vs %g)", straight.live_power, straight.power);
    CHECK(fabs(game.bags[game.nbags - 1].p.x - game.arrow_x) < 1e-12, "arrow starts at the release x");
    /* release persistence: ~2 s hold, then a 0.4 s fade */
    double a0 = game.arrow_age_ms;
    while (game.arrow_age_ms < 1950) frames(1);
    CHECK(game.arrow_on && arrow_alpha(game.arrow_age_ms, false) == 1, "still fully drawn 1.95 s after release (age was %g)", a0);
    while (game.arrow_on && game.arrow_age_ms < 2200) frames(1);
    double fade = arrow_alpha(game.arrow_age_ms, false);
    CHECK(game.arrow_on && fade > 0 && fade < 1, "fading at 2.2 s (%g)", fade);
    frames(20);
    CHECK(!game.arrow_on, "gone after 2.4 s");
    next_bag();
    SwingSeen right = bowling_swing(.65, .55, true);
    next_bag();
    SwingSeen left = bowling_swing(.65, -.55, true);
    CHECK(right.thrown && left.thrown && right.live_seen && left.live_seen, "leaning swings throw and preview");
    CHECK(right.yaw > 0.02 && left.yaw < -0.02, "swing right -> arrow right, swing left -> arrow left (%g, %g)", right.yaw, left.yaw);
    CHECK(fabs(right.yaw + left.yaw) < 1e-9, "left and right mirror");
    CHECK(fabs(right.yaw - right.bag_yaw) < 1e-12 && fabs(left.yaw - left.bag_yaw) < 1e-12, "arrow yaw is exactly the bag's launch yaw");
    CHECK(fabs(right.live_yaw - right.yaw) < 1e-9 && fabs(left.live_yaw - left.yaw) < 1e-9, "the preview direction matches the released direction");
    next_bag();
    SwingSeen hard = bowling_swing(.65, 1.1, true);
    CHECK(hard.yaw > right.yaw + 0.02, "a hard right swing points farther right than a slight one (%g > %g)", hard.yaw, right.yaw);
    CHECK(hard.yaw <= BYC_AIM_MAX + 1e-12, "within the aim limit");
    /* power variation */
    next_bag();
    SwingSeen soft = bowling_swing(.75, 0, true);
    next_bag();
    SwingSeen firm = bowling_swing(.55, 0, true);
    CHECK(soft.thrown && firm.thrown, "soft and firm swings throw");
    CHECK(soft.power < straight.power && straight.power < firm.power, "faster swing -> more power on the arrow (%g < %g < %g)", soft.power, straight.power, firm.power);
    CHECK(arrow_length(soft.power) < arrow_length(straight.power) && arrow_length(straight.power) < arrow_length(firm.power), "and a longer arrow");
    CHECK(fabs(soft.yaw) < 1e-12 && fabs(firm.yaw) < 1e-12, "power does not change direction");
    /* Left hand keeps the physical aim direction. */
    next_bag();
    SwingProfile lefty = input.slots[0].profile; lefty.left_handed = true;
    input_set_profile(&input, 0, lefty);
    SwingSeen lh = bowling_swing(.65, .55, true);
    CHECK(lh.thrown && lh.live_seen && fabs(lh.yaw - right.yaw) < 1e-9 && fabs(lh.live_yaw - right.yaw) < 1e-9, "LEFT HAND: the same motion preserves the arrow (%g vs %g)", lh.yaw, right.yaw);
    CHECK(fabs(lh.yaw - lh.bag_yaw) < 1e-12, "LEFT HAND: arrow still equals the launch");
    next_bag();
    SwingSeen lh_straight = bowling_swing(.65, 0, true);
    CHECK(fabs(lh_straight.yaw) < 1e-12 && fabs(lh_straight.power - straight.power) < 1e-9, "LEFT HAND: straight is straight, same power");
    lefty.left_handed = false; input_set_profile(&input, 0, lefty);
    /* a forward swing that is not released hides the preview once it ends */
    next_bag();
    while (game.arrow_on) frames(1);
    int bags = game.nbags;
    SwingSeen held = bowling_swing(.65, .55, false);
    CHECK(held.live_seen && game.nbags == bags, "held swing previews but does not throw");
    CHECK(!game.arrow_on, "a swing that ends without a throw hides the arrow");
    remote_buttons(0, -35, 0, 0);
    CHECK(game.nbags == bags && input.detector.phase == SWING_READY, "a late B-up cancels");
    /* shifted stance, straight swing: starts at the stance, still straight */
    next_bag();
    input.slots[0].stance = input.slots[0].stance_from = 0.3;
    SwingSeen shifted = bowling_swing(.65, 0, true);
    CHECK(shifted.thrown && fabs(shifted.live_x - 0.3) < 1e-9 && fabs(game.arrow_x - 0.3) < 1e-9 && fabs(shifted.yaw) < 1e-12, "stance 0.3: arrow starts there and stays straight (no auto-aim)");
    /* stepping / height / surface / pause hide it */
    CHECK(game.arrow_on, "arrow up");
    event(INPUT_PAUSE, 0);
    CHECK(!game.arrow_on, "pause hides the arrow");
    event(INPUT_CONFIRM, 0);

    /* screen geometry (what hud.c draws) */
    for (int wide = 0; wide < 2; wide++) {
        double aspect = wide ? 16.0 / 9 : 4.0 / 3;
        ArrowShape s0, sr, sl, sh, lo, mid, full, off;
        CHECK(arrow_shape(0, 0, 0.6, aspect, &s0) && arrow_shape(0, 0.05, 0.6, aspect, &sr) && arrow_shape(0, -0.05, 0.6, aspect, &sl) &&
              arrow_shape(0, 0.12, 0.6, aspect, &sh), "arrow projects");
        CHECK(fabs(s0.dx) < 1e-9 && s0.dy < -0.99, "straight from the centre stance points straight up-screen");
        CHECK(fabs(s0.ox - view_ui_width(aspect) / 2) < 1e-6 && s0.oy > 240 && s0.oy < 480, "starts at the bag (%.1f, %.1f)", s0.ox, s0.oy);
        CHECK(sr.dx > 0 && sl.dx < 0 && fabs(sr.dx + sl.dx) < 1e-9 && fabs(sr.dy - sl.dy) < 1e-9, "right/left mirror on screen");
        CHECK(sh.dx > sr.dx * 1.5, "hard right is clearly farther right (%g vs %g)", sh.dx, sr.dx);
        /* exact projected launch line: a point 9 m down the launch bearing lies on the arrow's line */
        double px, py; Vec3 far_pt = {0 + sin(0.12) * 9, BYC_REL_Y, BYC_REL_Z - cos(0.12) * 9};
        CHECK(view_project(far_pt, aspect, &px, &py), "far point projects");
        double cross = (px - sh.ox) * sh.dy - (py - sh.oy) * sh.dx;
        CHECK(fabs(cross) < 1e-6, "the arrow lies on the projected launch line (off by %g px)", cross);
        CHECK(fabs(sh.dx - sin(0.12)) > 1e-3, "which is not the raw yaw used as a screen angle");
        arrow_shape(0, 0.05, 0.05, aspect, &lo); arrow_shape(0, 0.05, 0.5, aspect, &mid); arrow_shape(0, 0.05, 1, aspect, &full);
        CHECK(lo.length == ARROW_MIN_PX && fabs(mid.length - 0.5 * ARROW_MAX_PX) < 1e-9 && full.length == ARROW_MAX_PX, "length = power on the full-power track (%g / %g / %g px)", lo.length, mid.length, full.length);
        CHECK(lo.length < mid.length && mid.length < full.length && full.length <= 80, "soft < medium < full, and compact");
        CHECK(fabs(lo.dx - full.dx) < 1e-12 && fabs(hypot(full.ex - full.ox, full.ey - full.oy) - ARROW_MAX_PX) < 1e-9 &&
              fabs(full.tx - full.ex) < 1e-9 && fabs(lo.ex - full.ex) < 1e-9, "the faint track always marks full power; the shaft fills it at full power");
        arrow_shape(0.45, 0, 0.6, aspect, &off);
        CHECK(off.ox > s0.ox + 5, "a stance to the right starts the arrow to the right");
    }
    CHECK(arrow_alpha(0, false) == 1 && arrow_alpha(ARROW_HOLD_MS, false) == 1 && arrow_alpha(ARROW_HOLD_MS + ARROW_FADE_MS / 2, false) == 0.5 &&
          arrow_alpha(ARROW_HOLD_MS + ARROW_FADE_MS, false) == 0 && arrow_alpha(99999, true) == 1, "2 s hold, 0.4 s fade; live preview never fades");
}

/* ---------------- WG-4: MUSIC, wind streaks ---------------- */
typedef struct {
    const char *files[8]; int nfiles; bool folder_ok; bool bad[8];
    char playing_name[MUSIC_NAME_MAX]; bool playing, paused, produced; double volume; int plays, stops;
    const char *folders[3]; int nfolders;
} FakeAudio;
static int fa_list(void *u, const char *folder, char names[][MUSIC_NAME_MAX], int cap) {
    FakeAudio *a = u; (void)folder;
    if (!a->folder_ok) return -1;
    int n = 0; for (int i = 0; i < a->nfiles && n < cap; i++) snprintf(names[n++], MUSIC_NAME_MAX, "%s", a->files[i]);
    return n;
}
static int fa_folders(void *u, char paths[][MUSIC_PATH_MAX], int cap) {
    FakeAudio *a = u; int n = 0; for (int i = 0; i < a->nfolders && n < cap; i++) snprintf(paths[n++], MUSIC_PATH_MAX, "%s", a->folders[i]);
    return n;
}
static bool fa_play(void *u, const char *folder, const char *name) {
    FakeAudio *a = u; (void)folder; a->plays++;
    snprintf(a->playing_name, sizeof a->playing_name, "%s", name);
    for (int i = 0; i < a->nfiles; i++) if (!strcmp(a->files[i], name) && a->bad[i]) { a->playing = false; a->produced = false; return true; }
    a->playing = true; a->produced = true; a->paused = false; return true;
}
static void fa_stop(void *u) { FakeAudio *a = u; a->playing = false; a->stops++; }
static void fa_pause(void *u, bool p) { ((FakeAudio *)u)->paused = p; }
static void fa_volume(void *u, double v) { ((FakeAudio *)u)->volume = v; }
static bool fa_playing(void *u) { return ((FakeAudio *)u)->playing; }
static bool fa_produced(void *u) { return ((FakeAudio *)u)->produced; }
static void test_music(void) {
    static Music music;
    FakeAudio fa; memset(&fa, 0, sizeof fa);
    fa.files[0] = "10 last.mp3"; fa.files[1] = "2 second.mp3"; fa.files[2] = "1 First.mp3"; fa.nfiles = 3; fa.folder_ok = true;
    fa.folders[0] = MUSIC_DEFAULT_FOLDER; fa.folders[1] = "sd:/music"; fa.nfolders = 2;
    MusicBackend be = {&fa, fa_list, fa_folders, fa_play, fa_stop, fa_pause, fa_volume, fa_playing, fa_produced};
    boot(141, NULL); game_menu_select(&game, MI_OK);
    music_init(&music, &be, game.save.music_on, game.save.music_volume, game.save.music_folder);
    game_set_music(&game, &music);
    CHECK(music.count == 3 && !strcmp(music.tracks[0], "1 First.mp3") && !strcmp(music.tracks[1], "2 second.mp3") && !strcmp(music.tracks[2], "10 last.mp3"), "tracks sorted like the arcade (numeric, case-insensitive)");
    CHECK(!music.enabled && fa.plays == 0 && fabs(fa.volume - 0.25) < 1e-12, "music is off by default at 25%% volume");
    CHECK(menu_index(MI_MUSIC) == 3, "title menu has MUSIC");
    game_menu_select(&game, MI_MUSIC);
    CHECK(game.ui == UI_MUSIC && game.menu.count == 6 && game.menu.items[0].id == MI_MUSIC_TOGGLE && game.menu.items[1].id == MI_MUSIC_NEXT &&
          game.menu.items[2].id == MI_MUSIC_FOLDER && game.menu.items[3].id == MI_MUSIC_REFRESH && game.menu.items[4].id == MI_MUSIC_VOLUME &&
          game.menu.items[5].id == MI_BACK, "MUSIC screen: toggle / NEXT TRACK / CHOOSE FOLDER / REFRESH / VOLUME / BACK");
    CHECK(!strcmp(game.menu.items[0].label, "PLAY MUSIC") && !strcmp(game.menu.items[4].value, "- 25% +"), "PLAY MUSIC, 25%%");
    CHECK(!strcmp(music_now(&music), "1 First.mp3"), "now: first track");
    game.save_dirty = false;
    event(INPUT_CONFIRM, 0); frames(1);
    CHECK(music.enabled && fa.playing && !strcmp(fa.playing_name, "1 First.mp3") && !strcmp(game.menu.items[0].label, "PAUSE MUSIC"), "PLAY MUSIC starts the first track");
    CHECK(game.save.music_on && game.save_dirty, "music on is saved");
    event(INPUT_CONFIRM, 0); frames(1);
    CHECK(!music.enabled && fa.paused && fa.stops == 0 && !strcmp(game.menu.items[0].label, "PLAY MUSIC"), "PAUSE MUSIC pauses in place");
    event(INPUT_CONFIRM, 0); frames(1);
    CHECK(music.enabled && !fa.paused && fa.plays == 1, "PLAY MUSIC resumes the same track");
    event(INPUT_NAV_DOWN, 0); event(INPUT_CONFIRM, 0);
    CHECK(!strcmp(fa.playing_name, "2 second.mp3") && !strcmp(music_now(&music), "2 second.mp3"), "NEXT TRACK");
    fa.playing = false; frames(1);
    CHECK(!strcmp(fa.playing_name, "10 last.mp3"), "a finished track advances to the next");
    fa.playing = false; frames(1);
    CHECK(!strcmp(fa.playing_name, "1 First.mp3"), "the playlist wraps");
    event(INPUT_NAV_DOWN, 0); event(INPUT_NAV_DOWN, 0); event(INPUT_NAV_DOWN, 0);
    CHECK(game.menu.items[game.menu.sel].id == MI_MUSIC_VOLUME, "on VOLUME");
    event(INPUT_NAV_RIGHT, 0); event(INPUT_ADJUST_PLUS, 0); frames(1);
    CHECK(fabs(music.volume - 0.45) < 1e-9 && fabs(fa.volume - 0.45) < 1e-9 && !strcmp(game.menu.items[4].value, "- 45% +"), "volume up twice = 45%%");
    for (int i = 0; i < 8; i++) event(INPUT_NAV_LEFT, 0);
    CHECK(music.volume == 0 && !strcmp(game.menu.items[4].value, "- 0% +"), "volume clamps at 0%%");
    for (int i = 0; i < 14; i++) event(INPUT_NAV_RIGHT, 0);
    frames(1);
    CHECK(music.volume == 1 && fabs(game.save.music_volume - 1) < 1e-12, "volume clamps at 100%% and is saved");
    /* folder choice + refresh */
    fa.files[0] = "b.mp3"; fa.files[1] = "a.mp3"; fa.nfiles = 2;
    game_menu_select(&game, MI_MUSIC_FOLDER); frames(1);
    CHECK(!strcmp(music.folder, "sd:/music") && !strcmp(game.save.music_folder, "sd:/music") && music.count == 2 && !strcmp(fa.playing_name, "a.mp3"), "CHOOSE FOLDER moves to the next folder, rescans, plays and saves");
    fa.files[2] = "c.mp3"; fa.nfiles = 3;
    game_menu_select(&game, MI_MUSIC_REFRESH);
    CHECK(music.count == 3 && music.index == 0, "REFRESH rescans");
    game_menu_select(&game, MI_MUSIC_FOLDER);
    CHECK(!strcmp(music.folder, MUSIC_DEFAULT_FOLDER), "CHOOSE FOLDER cycles back");
    /* unplayable files are skipped; all bad -> music turns off with the arcade's message */
    fa.bad[1] = true; fa.playing = false; frames(1);
    CHECK(!strcmp(fa.playing_name, "b.mp3"), "next");
    fa.playing = false; frames(1); frames(1);
    CHECK(music.enabled && fa.playing, "an unplayable file is skipped (%s)", fa.playing_name);
    fa.bad[0] = fa.bad[1] = fa.bad[2] = true; fa.playing = false;
    frames(8);
    CHECK(!music.enabled && !strcmp(music_now(&music), "Could not play these files.") && !game.save.music_on, "all files unplayable: music off, 'Could not play these files.'");
    CHECK(game.menu.items[0].id == MI_MUSIC_TOGGLE && !strcmp(game.menu.items[0].label, "PLAY MUSIC"), "menu label follows");
    /* empty / missing folder */
    fa.nfiles = 0; game_menu_select(&game, MI_MUSIC_REFRESH);
    CHECK(!strcmp(music_now(&music), "Add MP3s to your music folder.") && game.menu.items[0].disabled && game.menu.items[1].disabled, "empty folder: hint, PLAY and NEXT disabled");
    int plays = fa.plays; game.menu.sel = 0; event(INPUT_CONFIRM, 0);
    CHECK(!music.enabled && fa.plays == plays, "a disabled row does nothing");
    fa.folder_ok = false; game_menu_select(&game, MI_MUSIC_REFRESH);
    CHECK(!strcmp(music_now(&music), "Music folder unavailable."), "missing folder message");
    event(INPUT_BACK, 0);
    CHECK(game.ui == UI_TITLE, "B / BACK returns to the title");
    /* reachable from pause, keeps playing through play, returns to PAUSED */
    fa.folder_ok = true; fa.nfiles = 2; fa.bad[0] = fa.bad[1] = false;
    game_start(&game, true, false, false); while (game.phase != PH_AIM) frames(1);
    event(INPUT_PAUSE, 0);
    CHECK(menu_index(MI_MUSIC) >= 0, "pause menu has MUSIC");
    game_menu_select(&game, MI_MUSIC); game_menu_select(&game, MI_MUSIC_REFRESH); game_menu_select(&game, MI_MUSIC_TOGGLE);
    CHECK(fa.playing && music.enabled, "music started from the pause menu");
    game_menu_select(&game, MI_BACK);
    CHECK(game.ui == UI_PAUSE, "BACK returns to PAUSED");
    event(INPUT_CONFIRM, 0); frames(30);
    CHECK(fa.playing && game.phase == PH_AIM, "music keeps playing during play");
    /* settings survive a save round trip, version 1 saves still load */
    uint8_t buf[SAVE_MAX_BYTES]; size_t n = save_encode(&game.save, buf, sizeof buf);
    SaveData d; CHECK(n > 0 && save_decode(&d, buf, n) && d.music_on && d.music_volume == 1 && !strcmp(d.music_folder, MUSIC_DEFAULT_FOLDER), "music settings round-trip in save.dat");
    SaveData s2; save_defaults(&s2); s2.music_on = true; s2.music_volume = 0.4; snprintf(s2.music_folder, sizeof s2.music_folder, "sd:/music");
    n = save_encode(&s2, buf, sizeof buf);
    CHECK(save_decode(&d, buf, n) && d.music_on && fabs(d.music_volume - 0.4) < 1e-12 && !strcmp(d.music_folder, "sd:/music"), "folder + volume round-trip");
    uint8_t v1[64]; size_t k = 0;
    memcpy(v1, "BYCS", 4); k = 4; v1[k++] = 0; v1[k++] = 1; v1[k++] = 0; v1[k++] = 39;
    memcpy(v1 + k, buf + 8, 39); k += 39;
    uint32_t crc = save_crc32(v1, k); v1[k++] = (uint8_t)(crc >> 24); v1[k++] = (uint8_t)(crc >> 16); v1[k++] = (uint8_t)(crc >> 8); v1[k++] = (uint8_t)crc;
    CHECK(save_decode(&d, v1, k) && !d.music_on && d.music_volume == MUSIC_DEFAULT_VOLUME && !strcmp(d.music_folder, MUSIC_DEFAULT_FOLDER), "a version-1 save loads with default music settings");
    CHECK(music_compare_names("Track 2.mp3", "track 10.mp3") < 0 && music_compare_names("a.mp3", "B.mp3") < 0 && music_compare_names("x", "x") == 0, "name ordering");
    music_shutdown(&music); game_set_music(&game, NULL);
}

#include "wind_streaks.h"
static void test_wind_streaks(void) {
    static WindStreaks w; wind_streaks_init(&w);
    Wind calm = {0, 0, 0, 0}, breeze = {1, 0, 6, BYC_PI / 2}, gale = {-1, 0, 15, -BYC_PI / 2};
    wind_streaks_update(&w, 1.0 / 60, &calm, true);
    CHECK(!w.visible && w.active == 0, "calm air draws no streaks");
    wind_streaks_update(&w, 1.0 / 60, &breeze, false);
    CHECK(!w.visible, "hidden in menus");
    wind_streaks_update(&w, 1.0 / 60, &breeze, true);
    CHECK(w.visible && w.active == 12 && fabs(w.opacity - (.035 + .4 * .095)) < 1e-12, "6 mph: 12 streaks (%d), opacity %.4f", w.active, w.opacity);
    /* first streak from the JS LCG (seed 57413): x, y, z, phase, size */
    CHECK(fabs(w.streaks[0].x - ((((57413u * 1664525u + 1013904223u)) / 4294967296.0) - .5) * 5.6) < 1e-12, "same seed and draw order as wind-streaks.js");
    bool ok = true; double moved = 0, x0 = w.verts[0].x;
    for (int f = 0; f < 600; f++) {
        wind_streaks_update(&w, 1.0 / 60, &breeze, true);
        for (int i = 0; i < w.active * STREAK_VERTS; i++) {
            Vec3 v = w.verts[i];
            if (!isfinite(v.x) || !isfinite(v.y) || !isfinite(v.z) || v.y < 0.2 || v.y > 2.5 || fabs(v.z - w.verts[i - i % STREAK_VERTS].z) > 1e-9) ok = false;
        }
        if (f == 5) moved = w.verts[0].x - x0;
    }
    CHECK(ok, "streak vertices stay finite, above the grass and parallel to a crosswind");
    CHECK(moved > 0, "streaks drift downwind (+x) (%g)", moved);
    for (int i = 0; i < w.active; i++) {
        const Vec3 *v = &w.verts[i * STREAK_VERTS];
        if (!(v[0].x >= v[1].x && v[1].x >= v[2].x && v[2].x >= v[3].x)) ok = false;   /* tail trails upwind */
    }
    CHECK(ok, "each streak is two dashes trailing upwind");
    wind_streaks_update(&w, 1.0 / 60, &gale, true);
    CHECK(w.active == 24 && fabs(w.opacity - .13) < 1e-12, "15 mph: all 24 streaks at full opacity");
    const Vec3 *v = &w.verts[0];
    CHECK(v[0].x <= v[3].x, "wind from the other side flips the streaks");
}

int main(void) {
    physics_init();
    win_compute(&win);
    test_boot_and_menus();
    test_practice_boot_ignores_stale_b();
    test_save_roundtrip();
    test_scoring();
    test_wind_policy();
    test_human_throw_straight_from_stance();
    test_pause_and_disconnect();
    test_handedness_and_settings();
    test_practice();
    test_two_player_soak();
    test_career();
    test_clear_board_any_phase();
    test_pause_on_summary();
    test_combined_arrow();
    test_last_shot_gauge();
    test_deferred_release_stance();
    test_set_power_reaches_hole();
    test_practice_cancel_and_retry();
    test_swing_to_landing();
    test_version_tag();
    test_widescreen_layout();
    test_unlimited_practice();
    test_music();
    test_wind_streaks();
    printf("game tests: %d/%d checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
