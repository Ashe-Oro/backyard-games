#include "game.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const TEAM[2] = {"RED", "BLUE"};
static const double RX[2] = {0, 0};

static double clampd(double v, double a, double b) { return fmax(a, fmin(b, v)); }
static void copy(char *dst, size_t cap, const char *src) { snprintf(dst, cap, "%s", src); }

/* ---------------- small services ---------------- */
static void fx(Game *g, FxKind kind, Vec3 at, int ms) {
    if (g->fx_count >= 16) return;
    unsigned ch = g->input ? g->input->channels[g->input->active_slot] : 0;
    g->fx[(g->fx_head + g->fx_count++) % 16] = (Fx){kind, at, ms, ch};
}
bool game_pop_fx(Game *g, Fx *out) {
    if (!g->fx_count) return false;
    *out = g->fx[g->fx_head]; g->fx_head = (g->fx_head + 1) % 16; g->fx_count--;
    return true;
}
static Timer *timer_add(Game *g, TimerKind kind, double ms) {
    for (int i = 0; i < 8; i++) if (g->timers[i].kind == T_NONE) {
        memset(&g->timers[i], 0, sizeof(Timer));
        g->timers[i].kind = kind; g->timers[i].left_ms = ms;
        return &g->timers[i];
    }
    return NULL;
}
static void timers_clear(Game *g) { memset(g->timers, 0, sizeof(g->timers)); }
static void say(Game *g, const char *big, const char *small, int ms) {
    copy(g->msg_big, sizeof g->msg_big, big); copy(g->msg_small, sizeof g->msg_small, small ? small : "");
    g->msg_on = true;
    for (int i = 0; i < 8; i++) if (g->timers[i].kind == T_HIDE_MSG) g->timers[i].kind = T_NONE;
    timer_add(g, T_HIDE_MSG, ms);
}
static void mark_save(Game *g) {
    g->save.career_fight = g->fight; g->save.career_wins = g->wins;
    g->save.help_tucked = !g->help_shown;
    if (g->input) for (int s = 0; s < 2; s++) g->save.profile[s] = g->input->slots[s].profile;
    if (g->music) {
        g->save.music_on = g->music->enabled; g->save.music_volume = g->music->volume;
        copy(g->save.music_folder, sizeof g->save.music_folder, g->music->folder);
    }
    g->save_dirty = true;
}

int game_thrower(const Game *g) { return g->practice ? 0 : (g->k % 2 == 0 ? g->first : 1 - g->first); }
bool game_is_human(const Game *g, int t) { return t == 0 || !g->vs_cpu; }
double game_release_x(const Game *g, int t) { return RX[t] + (game_is_human(g, t) ? g->position[t] : 0); }
const char *game_team_name(const Game *g, int t) {
    if (!g->vs_cpu) return TEAM[t];
    return t == 0 ? "YOU" : g->career_match ? byc_roster[g->fight < BYC_RIVALS ? g->fight : BYC_RIVALS - 1].name : "CPU";
}
Lie game_lie(const Game *g) {
    Lie l; memset(&l, 0, sizeof l);
    l = g->banked;   /* practice only: in-the-hole bags already compacted out of the list */
    for (int i = 0; i < g->nbags; i++) {
        const Bag *b = &g->bags[i];
        if (b->state == BAG_IN) { l.pts[b->team] += BYC_POINTS_IN_HOLE; l.in_c[b->team]++; }
        else if (b->state == BAG_BOARD && !b->dead) { l.pts[b->team] += BYC_POINTS_ON_BOARD; l.on_c[b->team]++; }
    }
    return l;
}
bool game_menu_screen(const Game *g) { return g->ui != UI_NONE; }

SwingCalibrationLimits game_calibration_limits(const Game *g) {
    /* target: the NORMAL/SLICK centre-of-hole power from the ported physics (WIN mid).
     * floor/ceiling are PROVISIONAL until P1 hardware traces: 0.5 g and 2.0 g excess (the ±3 g rail
     * including gravity) through the same pendulum speed proxy. */
    SwingCalibrationLimits l = {g->win->w[0][SIDE_SLICK][ARC_NORMAL].mid, swing_speed(0.5), swing_speed(2.0)};
    return l;
}

/* ---------------- menus ---------------- */
static MenuItem *menu_add(Game *g, MenuItemId id, const char *label) {
    Menu *m = &g->menu;
    if (m->count >= GAME_MENU_MAX) return &m->items[GAME_MENU_MAX - 1];
    MenuItem *it = &m->items[m->count++];
    it->id = id; copy(it->label, sizeof it->label, label); it->value[0] = 0; it->disabled = false;
    return it;
}
static void menu_values(Game *g) {
    for (int i = 0; i < g->menu.count; i++) {
        MenuItem *it = &g->menu.items[i];
        const SwingProfile *p0 = g->input ? &g->input->slots[0].profile : &g->save.profile[0];
        const SwingProfile *p1 = g->input ? &g->input->slots[1].profile : &g->save.profile[1];
        double def = swing_default_profile().full_speed;
        switch (it->id) {
        case MI_HAND_RED: copy(it->value, sizeof it->value, p0->left_handed ? "< LEFT" : "RIGHT >"); break;
        case MI_HAND_BLUE: copy(it->value, sizeof it->value, p1->left_handed ? "< LEFT" : "RIGHT >"); break;
        case MI_POWER_RED: snprintf(it->value, sizeof it->value, "- %d%% +", (int)lround(p0->full_speed / def * 100)); break;
        case MI_POWER_BLUE: snprintf(it->value, sizeof it->value, "- %d%% +", (int)lround(p1->full_speed / def * 100)); break;
        case MI_CURL_RED: snprintf(it->value, sizeof it->value, "- %d%% +", (int)lround(p0->spin_strength * 100)); break;
        case MI_CURL_BLUE: snprintf(it->value, sizeof it->value, "- %d%% +", (int)lround(p1->spin_strength * 100)); break;
        case MI_WIND: copy(it->value, sizeof it->value, g->save.wind_on ? "ON" : "CALM"); break;
        case MI_MUSIC_TOGGLE:
            copy(it->label, sizeof it->label, g->music && g->music->enabled ? "PAUSE MUSIC" : "PLAY MUSIC");
            it->disabled = !g->music || !music_can_toggle(g->music);
            break;
        case MI_MUSIC_NEXT: it->disabled = !g->music || !music_can_next(g->music); break;
        case MI_MUSIC_FOLDER: case MI_MUSIC_REFRESH: it->disabled = !g->music; break;
        case MI_MUSIC_VOLUME:
            snprintf(it->value, sizeof it->value, "- %d%% +", (int)lround((g->music ? g->music->volume : g->save.music_volume) * 100));
            break;
        default: break;
        }
    }
}
static void menu_focus_effects(Game *g) {
    if (!g->input) return;
    MenuItemId id = g->menu.count ? g->menu.items[g->menu.sel].id : MI_BACK;
    if (id == MI_HAND_RED) input_focus_hand(g->input, 0);
    else if (id == MI_HAND_BLUE) input_focus_hand(g->input, 1);
    else input_blur_hand(g->input);
}
static void open_ui(Game *g, UiScreen ui) {
    g->ui = ui; g->menu.count = 0; g->menu.sel = 0;
    switch (ui) {
    case UI_TITLE:
        menu_add(g, MI_PRACTICE, "PRACTICE"); menu_add(g, MI_TWO_PLAYER, "2 PLAYER");
        menu_add(g, MI_TOURNAMENT, "TOURNAMENT"); menu_add(g, MI_MUSIC, "MUSIC");
        menu_add(g, MI_SETTINGS, "SETTINGS"); menu_add(g, MI_QUIT, "QUIT");
        break;
    case UI_PAUSE:
        menu_add(g, MI_RESUME, "RESUME");
        if (g->practice) menu_add(g, MI_CLEAR_BOARD, "CLEAR BOARD");
        menu_add(g, MI_HAND_RED, g->practice ? "HAND" : "RED HAND");
        if (!g->practice && !g->vs_cpu) menu_add(g, MI_HAND_BLUE, "BLUE HAND");
        menu_add(g, MI_MUSIC, "MUSIC"); menu_add(g, MI_SETTINGS, "SETTINGS");
        menu_add(g, MI_MAIN_MENU, "MAIN MENU"); menu_add(g, MI_QUIT, "QUIT");
        break;
    case UI_SETTINGS:
        menu_add(g, MI_HAND_RED, "RED HAND"); menu_add(g, MI_SETPOWER_RED, "RED SET POWER");
        menu_add(g, MI_POWER_RED, "RED POWER"); menu_add(g, MI_CURL_RED, "RED CURL");
        menu_add(g, MI_HAND_BLUE, "BLUE HAND"); menu_add(g, MI_SETPOWER_BLUE, "BLUE SET POWER");
        menu_add(g, MI_POWER_BLUE, "BLUE POWER"); menu_add(g, MI_CURL_BLUE, "BLUE CURL");
        menu_add(g, MI_WIND, "WIND"); menu_add(g, MI_RESET_SETTINGS, "RESET SETTINGS"); menu_add(g, MI_BACK, "BACK");
        break;
    case UI_CAREER_MAP:
        menu_add(g, MI_CONTINUE, g->fight >= BYC_RIVALS ? "VIEW TROPHY" : "CONTINUE");
        if (g->fight > 0 || g->wins > 0) menu_add(g, MI_NEW_CAREER, "NEW CAREER");
        menu_add(g, MI_MAIN_MENU, "MAIN MENU");
        break;
    case UI_INTRO: menu_add(g, MI_LETS_TOSS, "LET'S TOSS"); menu_add(g, MI_CIRCUIT_MAP, "CIRCUIT MAP"); break;
    case UI_CHAMPION: menu_add(g, MI_CIRCUIT_MAP, "CIRCUIT MAP"); menu_add(g, MI_NEW_CAREER, "NEW CAREER"); break;
    case UI_SUMMARY: menu_add(g, MI_NEXT, g->phase == PH_OVER ? "NEW MATCH" : "NEXT ROUND"); break;
    case UI_CAREER_RESULT:
        menu_add(g, MI_NEXT, g->winner == 0 ? (g->fight >= BYC_RIVALS ? "CLAIM THE CROWN" : "NEXT UP") : "TRY AGAIN");
        menu_add(g, MI_CIRCUIT_MAP, "CIRCUIT MAP");
        break;
    case UI_CONFIRM_QUIT: case UI_CONFIRM_NEW_CAREER: menu_add(g, MI_NO, "NO"); menu_add(g, MI_YES, "YES"); break;
    case UI_MUSIC:   /* arcade #arcade-audio: toggle, next, folder, refresh, volume, back */
        menu_add(g, MI_MUSIC_TOGGLE, "PLAY MUSIC"); menu_add(g, MI_MUSIC_NEXT, "NEXT TRACK");
        menu_add(g, MI_MUSIC_FOLDER, "CHOOSE FOLDER"); menu_add(g, MI_MUSIC_REFRESH, "REFRESH");
        menu_add(g, MI_MUSIC_VOLUME, "VOLUME"); menu_add(g, MI_BACK, "BACK");
        break;
    case UI_SETPOWER: case UI_STRAP: menu_add(g, MI_OK, "OK"); break;
    case UI_NONE: break;
    }
    menu_values(g);
    /* Switch the input layer now so the HAND row can take focus on the first frame. */
    if (g->input && ui != UI_SETPOWER && ui != UI_NONE && g->input->context != INPUT_MENU) input_set_context(g->input, INPUT_MENU);
    menu_focus_effects(g);
}

/* ---------------- wind policy (arcade controller host.wind(windy)) ---------------- */
static void apply_wind_setting(Game *g) {
    if (g->career_match) return;
    if (!g->save.wind_on) g->wind = (Wind){0, 0, 0, 0};
    else if (!g->wind.mph) g->wind = round_wind(false, -1, g->wind.x, &g->rng);
}

/* ---------------- match flow ---------------- */
static void clear_bags(Game *g) {
    g->nbags = 0; g->active = -1; g->nbefore = 0;
    g->live = g->banked = (Lie){{0, 0}, {0, 0}, {0, 0}};
}
/* Room for one more bag. Entries that have left the world (BAG_IN, BAG_REMOVED: never stepped,
 * collided with or drawn again) are squeezed out first, keeping insertion order and banking their
 * points; then the arrays double. Nothing visible is ever dropped and there is no bag limit. */
static bool reserve_bag(Game *g) {
    if (g->nbags < g->bag_cap) return true;
    int n = 0;
    for (int i = 0; i < g->nbags; i++) {
        const Bag *b = &g->bags[i];
        if (b->state == BAG_IN) { g->banked.pts[b->team] += BYC_POINTS_IN_HOLE; g->banked.in_c[b->team]++; continue; }
        if (b->state == BAG_REMOVED) continue;
        if (n != i) g->bags[n] = *b;
        n++;
    }
    g->nbags = n;
    if (g->nbags < g->bag_cap) return true;
    int cap = g->bag_cap ? g->bag_cap * 2 : 64;
    if (g->bag_cap_limit > 0 && cap > g->bag_cap_limit) return false;
    Bag *bags = realloc(g->bags, (size_t)cap * sizeof *bags);
    if (bags) g->bags = bags;
    BagState *before = bags ? realloc(g->before, (size_t)cap * sizeof *before) : NULL;
    if (before) g->before = before;
    if (!bags || !before) return false;
    g->bag_cap = cap;
    return true;
}

static void sync_input_slot(Game *g, int t) {
    if (!g->input) return;
    InputSlot *s = &g->input->slots[t];
    s->height = g->heights[t];
    s->sticky = g->side[t] == SIDE_STICKY;
}

static void show_menu(Game *g) {
    timers_clear(g); g->active = -1; g->nbefore = 0; g->paused = false;
    g->phase = PH_MENU; g->career_match = false; g->resume_pending = false;
    clear_bags(g);
    g->arrow_on = g->arrow_live = false;
    open_ui(g, UI_TITLE);
}

static void begin_inning(Game *g, double delay) {
    g->wind = round_wind(g->practice, g->career_match ? byc_roster[g->fight].circuit : -1, g->inning > 1 ? g->wind.x : 0, &g->rng);
    apply_wind_setting(g);
    clear_bags(g); g->k = 0; g->phase = PH_WAIT;
    timer_add(g, T_NEXT_TURN, delay);
}

void game_start(Game *g, bool practice, bool vs_cpu, bool career) {
    timers_clear(g); g->paused = false; g->resume_pending = false;
    g->vs_cpu = vs_cpu; g->exact = false;
    g->career_match = career;
    g->practice = practice; g->practice_throws = g->practice_in = g->practice_on = 0;
    g->heights[0] = g->heights[1] = 1;
    g->rival_color = career ? byc_roster[g->fight].color : 0x2f5fe8;
    g->position[0] = g->position[1] = 0; g->spin = 0; g->arc = ARC_NORMAL;
    g->scores[0] = g->scores[1] = 0; g->inning = 1; g->winner = -1;
    g->first = practice ? 0 : (rng_next(&g->rng) < 0.5 ? 0 : 1);   /* coin toss */
    g->ui = UI_NONE; g->menu.count = 0;
    g->arrow_on = g->arrow_live = false; g->cpu_planning = false;
    fx(g, FX_DOG_RESET, (Vec3){0, 0, 0}, 0);
    if (g->input) {
        input_start_match(g->input);
        for (int s = 0; s < 2; s++) {
            InputSlot *is = &g->input->slots[s];
            is->stance = is->stance_from = is->aim = 0;
            sync_input_slot(g, s);
        }
        g->pass_and_play = !practice && !vs_cpu && g->input->channels[1] == g->input->channels[0];
    }
    char big[24]; snprintf(big, sizeof big, "%s THROWS FIRST", TEAM[g->first]);
    say(g, big, "", 1500);
    begin_inning(g, 900);
}

static void next_turn(Game *g) {
    if (g->phase == PH_MENU || g->phase == PH_INTRO || g->phase == PH_CHAMPION) return;
    if (!g->practice && g->k >= BYC_BAGS_PER_INNING) { g->phase = PH_SETTLE; return; }
    int t = game_thrower(g);
    g->aim = 0; g->arc = (BagArc)g->heights[t]; g->meter = 0;
    if (game_is_human(g, t)) {
        g->phase = PH_AIM;
        g->spin = 0;
        if (g->input) { sync_input_slot(g, t); g->input->slots[t].aim = 0; input_next_turn(g->input, (unsigned)t); }
    } else {
        CpuShot c = cpu_casual(g->win, &g->wind, &g->rng);
        g->side[1] = c.side; g->arc = ARC_NORMAL; g->spin = 0;
        g->cpu = c; g->cpu_t = 0;
        if (g->career_match) { cpu_plan_begin(&g->plan, &byc_roster[g->fight], g->bags, g->nbags, g->scores, &g->wind); g->cpu_planning = true; }
        g->phase = PH_CPU;
    }
}

static bool throw_bag(Game *g, double power, double drift, int team) {
    if (g->paused || g->connect_prompt) return false;   /* frozen worlds never take a throw */
    if (!(g->phase == PH_AIM || g->phase == PH_CPU)) return false;
    if (!reserve_bag(g)) { say(g, "OUT OF MEMORY", "HOME: CLEAR BOARD", 2000); return false; }
    BagSide side = g->side[team];
    bool human = game_is_human(g, team);
    double actual = human ? physical_power(g->win, team, side, g->arc, power) : power;
    Bag *b = &g->bags[g->nbags];
    bag_new(b, team, side);
    double rx = game_release_x(g, team);
    /* Wii human throws start straight down-range from the stance (owner directive: no auto-aim
     * at the hole). The CPU keeps 1.0.3's baseYaw(RX[1]) line; with RX = 0 both bases are 0. */
    double yaw_base = base_yaw(RX[team]);
    bag_launch_yaw(b, rx, yaw_base, actual, g->aim, drift, g->arc, g->spin);
    b->release_yaw = yaw_base + g->aim;
    if (!human) b->spin += drift * 2;
    b->tumble = 0.6 + rng_next(&g->rng);
    g->nbefore = g->nbags;
    for (int i = 0; i < g->nbags; i++) g->before[i] = g->bags[i].state;
    g->active = g->nbags++;
    g->phase = PH_FLYING; g->meter = power;
    if (human) {
        /* 1.0.7 showShotArrow: the actual launch yaw and physical power, from the release x. */
        g->arrow_on = true; g->arrow_live = false; g->arrow_age_ms = 0;
        g->arrow_power = actual; g->arrow_yaw = atan2(b->v.x, -b->v.z); g->arrow_x = b->p.x;
        fx(g, FX_RUMBLE, b->p, 60);
    }
    return true;
}

bool game_throw(Game *g, double power, double aim, double spin) {
    if (g->paused || g->connect_prompt || g->phase != PH_AIM || !game_is_human(g, game_thrower(g)) || g->ui != UI_NONE) return false;
    g->aim = clampd(aim, -BYC_AIM_MAX, BYC_AIM_MAX);
    g->spin = BYC_HUMAN_SPIN ? clampd(spin, -1, 1) : 0;
    return throw_bag(g, clampd(power, 0, 1), 0, game_thrower(g));
}

static void team_list_add(char *out, size_t cap, int team) {
    size_t len = strlen(out);
    snprintf(out + len, cap - len, "%s%s", len ? " + " : "", TEAM[team]);
}

static void after_throw(Game *g) {
    Bag *b = &g->bags[g->active]; g->active = -1;
    int nin = 0, noff = 0;
    char in_list[64] = "", off_list[64] = "";
    for (int i = 0; i < g->nbefore; i++) {
        BagState s = g->before[i], now = g->bags[i].state;
        if (s != BAG_IN && now == BAG_IN) { nin++; team_list_add(in_list, sizeof in_list, g->bags[i].team); }
        if (s == BAG_BOARD && !bag_on_deck(&g->bags[i]) && now != BAG_IN) { noff++; team_list_add(off_list, sizeof off_list, g->bags[i].team); }
    }
    Vec3 hole = byc_board.hole_world; hole.y += 0.1;
    if (b->state == BAG_IN || nin) fx(g, FX_CELEBRATE, hole, 0);
    if (b->state == BAG_IN) { say(g, "CORNHOLE!", "3 POINTS", 1400); fx(g, FX_HOLE_BURST, hole, 0); fx(g, FX_RUMBLE, hole, 260); }
    else if (b->state == BAG_BOARD) {
        say(g, hole_dist(b->lp) < BYC_HOLE_R + BYC_BAG_R * 0.6 ? "HANGER!" : "ON THE BOARD", "1 POINT", 1400);
        fx(g, FX_DUST, b->p, 0); fx(g, FX_RUMBLE, b->p, 90);
    }
    else if (b->dead) say(g, "GROUNDER", "GRASS FIRST - NO SCORE", 1300);
    else say(g, b->off_board ? "OFF THE BOARD" : "MISS", "0", 1100);
    if (nin) {
        Timer *t = timer_add(g, T_SAY_LATER, 900);
        if (t) { copy(t->big, sizeof t->big, "KNOCKED IN!"); snprintf(t->small, sizeof t->small, "%.36s BAG +3", in_list); t->say_ms = 1400; }
        fx(g, FX_HOLE_BURST, hole, 0);
    } else if (noff) {
        Timer *t = timer_add(g, T_SAY_LATER, 900);
        if (t) { copy(t->big, sizeof t->big, "KNOCKED OFF!"); snprintf(t->small, sizeof t->small, "%.36s BAG", off_list); t->say_ms = 1400; }
    }
    if (g->practice) { g->practice_throws++; g->practice_in += b->state == BAG_IN; g->practice_on += b->state == BAG_BOARD; }
    g->k++; g->phase = PH_WAIT;
    apply_wind_setting(g);
    timer_add(g, T_NEXT_TURN, nin || noff ? 1500 : 750);
}

static void career_result(Game *g, int win) {
    if (win == 0) { g->fight++; g->wins++; mark_save(g); }
    open_ui(g, UI_CAREER_RESULT);
}

static void score_inning(Game *g) {
    Lie l = game_lie(g);
    int net = abs(l.pts[0] - l.pts[1]);
    int scorer = l.pts[0] > l.pts[1] ? 0 : l.pts[1] > l.pts[0] ? 1 : -1;
    bool bust = false;
    if (scorer >= 0) {
        g->scores[scorer] += net;
        if (g->exact && g->scores[scorer] > BYC_TARGET_SCORE) { g->scores[scorer] = BYC_EXACT_BUST_TO; bust = true; }
        g->first = scorer;   /* last scoring team throws first; a wash keeps the order */
    }
    int win = scorer >= 0 && (g->exact ? g->scores[scorer] == BYC_TARGET_SCORE : g->scores[scorer] >= BYC_TARGET_SCORE) ? scorer : -1;
    g->round = (RoundResult){l, net, scorer, g->inning, {g->scores[0], g->scores[1]}, bust};
    g->phase = win >= 0 ? PH_OVER : PH_SUMMARY; g->winner = win;
    open_ui(g, UI_SUMMARY);
    if (win >= 0 && g->career_match) career_result(g, win);
}

static void advance(Game *g) {
    if (g->phase == PH_SUMMARY) { g->ui = UI_NONE; g->inning++; begin_inning(g, 300); }
    else if (g->phase == PH_OVER) { clear_bags(g); show_menu(g); }
}

static void career_map(Game *g) {
    g->career_match = false; g->phase = PH_MENU; clear_bags(g); timers_clear(g);
    open_ui(g, UI_CAREER_MAP);
}
static void career_intro(Game *g) { g->phase = PH_INTRO; open_ui(g, UI_INTRO); }
static void champion(Game *g) {
    g->phase = PH_CHAMPION; open_ui(g, UI_CHAMPION);
    Vec3 hole = byc_board.hole_world; fx(g, FX_HOLE_BURST, hole, 0);
}

static void next_turn(Game *g);
void game_clear_board(Game *g) {
    if (!g->practice || g->phase == PH_MENU) return;
    /* Practice is always WAIT, AIM or FLYING. Whatever was in progress (a bag in the air, the
     * pause before the next bag, a pending KNOCKED IN! callout) belonged to the cleared board, so
     * drop it and hand the player a fresh bag. Throw/in/on stats are kept. */
    clear_bags(g);
    for (int i = 0; i < 8; i++) if (g->timers[i].kind == T_NEXT_TURN || g->timers[i].kind == T_SAY_LATER) g->timers[i].kind = T_NONE;
    g->arrow_on = g->arrow_live = false;
    g->phase = PH_WAIT;
    next_turn(g);
}

/* ---------------- settings / calibration ---------------- */
static void set_profile(Game *g, int slot, SwingProfile p) {
    if (g->input) input_set_profile(g->input, (unsigned)slot, p);
    g->save.profile[slot] = p;
    mark_save(g);
    menu_values(g);
}
static void begin_setpower(Game *g, int slot) {
    g->settings_return = g->ui == UI_SETTINGS ? g->settings_return : UI_TITLE;
    g->setpower_slot = slot; g->setpower_count = 0; copy(g->setpower_note, sizeof g->setpower_note, "");
    open_ui(g, UI_SETPOWER);
    if (g->input) {
        if (g->phase == PH_MENU) input_start_match(g->input);
        input_next_turn(g->input, (unsigned)slot);
        input_set_context(g->input, INPUT_PLAY);
        g->resume_pending = true;
        if (!input_begin_calibration(g->input, game_calibration_limits(g))) copy(g->setpower_note, sizeof g->setpower_note, "TRY AGAIN");
    }
}
static void end_setpower(Game *g) {
    if (g->input) input_cancel_calibration(g->input);
    open_ui(g, UI_SETTINGS);
}

/* ---------------- menu actions ---------------- */
static void resume_play(Game *g) {
    g->paused = false; g->resume_pending = true;
    /* HOME on a round summary / FINAL / tournament result goes back to that screen: it holds the
     * only NEXT ROUND / NEW MATCH / NEXT UP button. */
    if (g->pause_return != UI_NONE && g->phase != PH_MENU) { UiScreen back_to = g->pause_return; g->pause_return = UI_NONE; open_ui(g, back_to); return; }
    g->ui = UI_NONE; g->pause_return = UI_NONE;
    /* SET POWER from the pause menu may have moved the input layer to another slot. */
    int t = game_thrower(g);
    if (g->input && g->phase == PH_AIM && game_is_human(g, t)) input_next_turn(g->input, (unsigned)t);
}
static void pause_game(Game *g) {
    if (g->phase == PH_MENU || g->phase == PH_INTRO || g->phase == PH_CHAMPION) return;
    g->paused = true;
    g->pause_return = g->ui == UI_SUMMARY || g->ui == UI_CAREER_RESULT ? g->ui : UI_NONE;
    g->arrow_on = g->arrow_live = false;
    open_ui(g, UI_PAUSE);
}

void game_menu_select(Game *g, MenuItemId id) {
    UiScreen ui = g->ui;
    switch (id) {
    case MI_PRACTICE: game_start(g, true, false, false); break;
    case MI_TWO_PLAYER: game_start(g, false, false, false); break;
    case MI_TOURNAMENT: career_map(g); break;
    case MI_SETTINGS: g->settings_return = ui; open_ui(g, UI_SETTINGS); break;
    case MI_QUIT: open_ui(g, UI_CONFIRM_QUIT); g->settings_return = ui; break;
    case MI_YES:
        if (ui == UI_CONFIRM_QUIT) g->quit_requested = true;
        else if (ui == UI_CONFIRM_NEW_CAREER) { g->fight = 0; g->wins = 0; mark_save(g); career_map(g); }
        break;
    case MI_NO:
        if (ui == UI_CONFIRM_NEW_CAREER) { if (g->phase == PH_CHAMPION) open_ui(g, UI_CHAMPION); else career_map(g); }
        else open_ui(g, g->settings_return == UI_PAUSE ? UI_PAUSE : g->phase == PH_MENU ? UI_TITLE : UI_PAUSE);
        break;
    case MI_RESUME: resume_play(g); break;
    case MI_CLEAR_BOARD: game_clear_board(g); resume_play(g); break;
    case MI_MAIN_MENU: g->pause_return = UI_NONE; show_menu(g); break;
    case MI_MUSIC: g->music_return = ui; open_ui(g, UI_MUSIC); break;
    case MI_MUSIC_TOGGLE: if (g->music) { music_toggle(g->music); menu_values(g); } break;
    case MI_MUSIC_NEXT: if (g->music) { music_next(g->music); menu_values(g); } break;
    case MI_MUSIC_FOLDER: if (g->music) { music_choose_folder(g->music); menu_values(g); } break;
    case MI_MUSIC_REFRESH: if (g->music) { music_refresh(g->music); menu_values(g); } break;
    case MI_MUSIC_VOLUME: break;
    case MI_HAND_RED: case MI_HAND_BLUE: {
        int s = id == MI_HAND_RED ? 0 : 1;
        SwingProfile p = g->input ? g->input->slots[s].profile : g->save.profile[s];
        p.left_handed = !p.left_handed;
        set_profile(g, s, p);
        if (g->input) input_cancel_calibration(g->input);
        break;
    }
    case MI_SETPOWER_RED: begin_setpower(g, 0); break;
    case MI_SETPOWER_BLUE: begin_setpower(g, 1); break;
    case MI_WIND:
        g->save.wind_on = !g->save.wind_on; mark_save(g); menu_values(g);
        if (g->phase != PH_MENU && g->phase != PH_INTRO && g->phase != PH_CHAMPION) apply_wind_setting(g);
        break;
    case MI_RESET_SETTINGS:
        for (int s = 0; s < 2; s++) set_profile(g, s, swing_default_profile());
        g->save.wind_on = false; mark_save(g); menu_values(g);
        break;
    case MI_BACK:
        if (ui == UI_SETTINGS) open_ui(g, g->settings_return == UI_PAUSE ? UI_PAUSE : UI_TITLE);
        else if (ui == UI_MUSIC) open_ui(g, g->music_return == UI_PAUSE ? UI_PAUSE : UI_TITLE);
        break;
    case MI_CONTINUE: if (g->fight >= BYC_RIVALS) champion(g); else career_intro(g); break;
    case MI_NEW_CAREER: open_ui(g, UI_CONFIRM_NEW_CAREER); break;
    case MI_LETS_TOSS: game_start(g, false, true, true); break;
    case MI_CIRCUIT_MAP: career_map(g); break;
    case MI_NEXT:
        if (ui == UI_CAREER_RESULT) { if (g->fight >= BYC_RIVALS && g->winner == 0) champion(g); else career_intro(g); }
        else advance(g);
        break;
    case MI_OK:
        if (ui == UI_STRAP) open_ui(g, UI_TITLE);
        else if (ui == UI_SETPOWER) end_setpower(g);
        break;
    case MI_POWER_RED: case MI_POWER_BLUE: case MI_CURL_RED: case MI_CURL_BLUE: break;
    }
}

static void adjust(Game *g, int dir) {
    if (!g->menu.count) return;
    MenuItemId id = g->menu.items[g->menu.sel].id;
    int s = (id == MI_POWER_BLUE || id == MI_CURL_BLUE || id == MI_HAND_BLUE) ? 1 : 0;
    SwingProfile p = g->input ? g->input->slots[s].profile : g->save.profile[s];
    double def = swing_default_profile().full_speed;
    if (id == MI_POWER_RED || id == MI_POWER_BLUE) {
        SwingCalibrationLimits l = game_calibration_limits(g);
        /* Higher full speed = harder to reach full power. Shown as % of the default. */
        p.full_speed = clampd(p.full_speed + dir * def * 0.05, l.full_speed_floor, l.clip_ceiling);
        set_profile(g, s, p);
    } else if (id == MI_CURL_RED || id == MI_CURL_BLUE) {
        p.spin_strength = clampd(round((p.spin_strength + dir * 0.1) * 10) / 10, 0, 1);
        set_profile(g, s, p);
    } else if (id == MI_WIND) game_menu_select(g, MI_WIND);
    else if (id == MI_MUSIC_VOLUME && g->music) { music_volume_step(g->music, dir); menu_values(g); }
}

static void back(Game *g) {
    switch (g->ui) {
    case UI_SETTINGS: case UI_MUSIC: game_menu_select(g, MI_BACK); break;
    case UI_PAUSE: resume_play(g); break;
    case UI_CAREER_MAP: show_menu(g); break;
    case UI_INTRO: case UI_CHAMPION: career_map(g); break;
    case UI_CONFIRM_QUIT: case UI_CONFIRM_NEW_CAREER: game_menu_select(g, MI_NO); break;
    case UI_SETPOWER: end_setpower(g); break;
    case UI_CAREER_RESULT: career_map(g); break;
    default: break;
    }
}

/* ---------------- input events ---------------- */
void game_input(Game *g, const InputEvent *e) {
    switch (e->action) {
    case INPUT_CONNECT_REMOTE:
        g->connect_prompt = true; g->connect_remote = (unsigned)e->value;
        g->connect_offer_reassign = e->slot == 1 && e->channel == 1;
        return;
    case INPUT_HELP:
        g->help_shown = !g->help_shown; mark_save(g);
        return;
    case INPUT_PROFILE_CHANGED: {
        if (!g->input || e->slot > 1) return;
        g->save.profile[e->slot] = g->input->slots[e->slot].profile;
        mark_save(g); menu_values(g);
        return;
    }
    case INPUT_CAL_MORE: g->setpower_count = (int)e->value; copy(g->setpower_note, sizeof g->setpower_note, ""); return;
    case INPUT_CAL_RETRY: g->setpower_count = 0; copy(g->setpower_note, sizeof g->setpower_note, "TRY AGAIN"); return;
    case INPUT_CAL_DONE:
        g->setpower_count = SWING_CAL_TOSSES; copy(g->setpower_note, sizeof g->setpower_note, "POWER SET!");
        mark_save(g);
        return;
    case INPUT_PAUSE:
        if (g->ui == UI_SETPOWER) { end_setpower(g); return; }
        if (g->ui == UI_NONE || g->ui == UI_SUMMARY || g->ui == UI_CAREER_RESULT) pause_game(g);
        else if (g->ui == UI_PAUSE) resume_play(g);
        else back(g);
        /* HOME always pauses the input layer; when no pause menu opened, release it again. */
        if (g->ui != UI_PAUSE) g->resume_pending = true;
        return;
    default: break;
    }
    if (g->connect_prompt) return;
    if (g->ui != UI_NONE) {
        Menu *m = &g->menu;
        switch (e->action) {
        case INPUT_NAV_UP: if (m->count) { m->sel = (m->sel + m->count - 1) % m->count; menu_focus_effects(g); } break;
        case INPUT_NAV_DOWN: if (m->count) { m->sel = (m->sel + 1) % m->count; menu_focus_effects(g); } break;
        case INPUT_NAV_LEFT: case INPUT_ADJUST_MINUS: adjust(g, -1); break;
        case INPUT_NAV_RIGHT: case INPUT_ADJUST_PLUS: adjust(g, 1); break;
        case INPUT_CONFIRM: if (m->count && g->ui != UI_SETPOWER && !m->items[m->sel].disabled) game_menu_select(g, m->items[m->sel].id); break;
        case INPUT_BACK: if (g->ui != UI_SETPOWER) back(g); break;
        default: break;
        }
        return;
    }
    int t = game_thrower(g);
    if (g->phase != PH_AIM || !game_is_human(g, t) || g->paused) return;
    switch (e->action) {
    case INPUT_THROW:
        game_throw(g, e->toss.power, e->toss.aim, e->toss.spin);
        break;
    case INPUT_HEIGHT: g->arc = (BagArc)(int)e->value; g->heights[t] = (int)e->value; g->arrow_on = false; break;
    case INPUT_SURFACE: g->side[t] = e->value != 0 ? SIDE_STICKY : SIDE_SLICK; g->arrow_on = false; break;
    case INPUT_STEP: g->arrow_on = false; break;  /* eased stance read each frame via input_stance */
    case INPUT_RESET_BAG: g->position[t] = 0; g->aim = 0; g->spin = 0; g->arrow_on = false; g->meter = 0; break;
    default: break;
    }
}

/* ---------------- frame ---------------- */
static InputContext wanted_context(const Game *g) {
    if (g->ui == UI_SETPOWER) return INPUT_PLAY;
    if (g->ui != UI_NONE) return INPUT_MENU;
    if (g->phase == PH_AIM && game_is_human(g, game_thrower(g))) return INPUT_PLAY;
    return INPUT_DISABLED;
}

static void run_timers(Game *g, double dt_ms) {
    for (int i = 0; i < 8; i++) {
        Timer *t = &g->timers[i];
        if (t->kind == T_NONE) continue;
        t->left_ms -= dt_ms;
        if (t->left_ms > 0) continue;
        Timer fire = *t; t->kind = T_NONE;
        if (fire.kind == T_NEXT_TURN) next_turn(g);
        else if (fire.kind == T_HIDE_MSG) g->msg_on = false;
        else if (fire.kind == T_SAY_LATER) say(g, fire.big, fire.small, fire.say_ms);
    }
}

void game_update(Game *g, double dt, uint64_t now_ms) {
    dt = fmin(dt, 0.05);
    g->now_ms = now_ms;
    Input *in = g->input;

    if (in) {
        if (g->connect_prompt) {
            if (!in->disconnect_pause) g->connect_prompt = false;                /* reassigned: USE REMOTE 1 */
            else if (in->connected[in->channels[in->active_slot]] && input_resume(in)) g->connect_prompt = false;
        }
        InputContext want = wanted_context(g);
        if (in->context != want) input_set_context(in, want);
        if (g->resume_pending && !g->connect_prompt) {
            if (!in->paused) g->resume_pending = false;
            else if (input_resume(in)) g->resume_pending = false;
        }
        if (g->phase == PH_AIM && game_is_human(g, game_thrower(g)) && g->ui == UI_NONE) {
            int t = game_thrower(g);
            g->position[t] = input_stance(in, (unsigned)t, now_ms);
            g->aim = in->slots[t].aim;  /* fine trim preview (− / +) */
            /* 1.0.7 host.preview: while the forward swing is in progress the one arrow follows the
             * launch direction and power a release would give; a swing that ends without a throw
             * hides it again (finishStroke). */
            double power, aim;
            if (!g->paused && !g->connect_prompt && !in->calibrating && swing_preview(&in->detector, &in->slots[t].profile, &power, &aim)) {
                g->arrow_on = g->arrow_live = true; g->arrow_age_ms = 0;
                g->arrow_power = physical_power(g->win, t, g->side[t], g->arc, clampd(power, 0, 1));
                g->arrow_yaw = game_launch_yaw(g, t, aim); g->arrow_x = game_release_x(g, t);
            } else if (g->arrow_live) g->arrow_on = g->arrow_live = false;
        } else if (g->arrow_live) g->arrow_on = g->arrow_live = false;
    }

    bool frozen = g->paused || g->connect_prompt || (g->ui == UI_PAUSE) || (g->ui == UI_CONFIRM_QUIT && g->phase != PH_MENU);
    if (!frozen) {
        g->time += dt;
        run_timers(g, dt * 1000);
        if (g->phase == PH_CPU) {
            if (g->cpu_planning) {
                if (cpu_plan_run(&g->plan, g->cpu_step_budget, &g->rng)) {
                    g->cpu_planning = false;
                    g->cpu = g->plan.result;
                    g->arc = g->plan.result.arc; g->spin = g->plan.result.spin; g->side[1] = g->plan.result.side;
                }
            }
            g->cpu_t += dt;
            if (g->cpu_planning && g->cpu_t > 0.69) g->cpu_t = 0.69;   /* the windup holds until the plan is ready */
            if (!g->cpu_planning) {
                g->aim += (g->cpu.aim - g->aim) * fmin(1, dt * 3);
                if (g->cpu_t > 0.7) g->meter = fmin(g->cpu.power, (g->cpu_t - 0.7) / 0.5 * g->cpu.power);
                if (g->cpu_t > 1.3) { g->aim = g->cpu.aim; throw_bag(g, g->cpu.power, g->cpu.drift, 1); }
            }
        }
        /* fixed 240 Hz steps; index.html keeps a fractional accumulator */
        g->acc += dt;
        while (g->acc >= 1.0 / 240) { g->acc -= 1.0 / 240; physics_substep(g->bags, g->nbags, &g->wind); }
        if (g->phase == PH_FLYING && g->active >= 0 && bags_settled(g->bags, g->nbags)) after_throw(g);
        if (g->phase == PH_SETTLE && bags_settled(g->bags, g->nbags)) score_inning(g);
        /* index.html frame loop, render-only: flight yaw is absolute; on deck it continues from the
         * landing yaw and drifts with spin; visual spin decays while sliding. */
        for (int i = 0; i < g->nbags; i++) {
            Bag *b = &g->bags[i];
            if (b->state == BAG_FLY) b->yaw = b->release_yaw + b->spin * b->t;
            if (b->state == BAG_SLIDE) { b->yaw += b->spin * dt * 0.3; b->spin *= pow(0.1, dt); }
        }
    }
    /* scoreboard recount happens only when every bag rests (arcade controller) */
    if (bags_settled(g->bags, g->nbags)) g->live = game_lie(g);

    if (g->arrow_on && !g->arrow_live && !frozen) {
        g->arrow_age_ms += dt * 1000;
        if (g->arrow_age_ms > ARROW_HOLD_MS + ARROW_FADE_MS) g->arrow_on = false;
    }
    if (g->music) {
        music_update(g->music);
        if (g->music->dirty) { g->music->dirty = false; mark_save(g); if (g->ui == UI_MUSIC) menu_values(g); }
    }
}

double game_launch_yaw(const Game *g, int team, double aim) {
    (void)g;
    return base_yaw(RX[team]) + clampd(aim, -BYC_AIM_MAX, BYC_AIM_MAX);
}
void game_set_music(Game *g, Music *music) { g->music = music; }
void game_free(Game *g) {
    free(g->bags); free(g->before);
    g->bags = NULL; g->before = NULL; g->bag_cap = g->nbags = g->nbefore = 0; g->active = -1;
}
void game_init(Game *g, Input *input, const WinTable *win, uint32_t seed, const SaveData *loaded, bool save_ok) {
    Bag *bags = g->bags; BagState *before = g->before; int cap = g->bag_cap;   /* keep the buffers */
    memset(g, 0, sizeof(*g));
    g->bags = bags; g->before = before; g->bag_cap = cap;
    reserve_bag(g);   /* first block; later growth happens between throws */
    g->input = input; g->win = win; g->active = -1;
    rng_seed(&g->rng, seed);
    if (loaded) g->save = *loaded; else save_defaults(&g->save);
    g->save_ok = save_ok;
    g->fight = g->save.career_fight; g->wins = g->save.career_wins;
    g->help_shown = !g->save.help_tucked;
    g->side[0] = g->side[1] = SIDE_SLICK;
    g->heights[0] = g->heights[1] = 1;
    g->cpu_step_budget = 30000;
    if (input) for (int s = 0; s < 2; s++) input_set_profile(input, (unsigned)s, g->save.profile[s]);
    g->phase = PH_MENU;
    open_ui(g, UI_STRAP);
}
