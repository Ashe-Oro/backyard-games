/* Match flow, modes, menus and HUD state for the Wii build. Platform-free (host-testable).
 * Ports index.html (startMatch/beginInning/nextTurn/throwBag/afterThrow/scoreInning/advance,
 * career screens) and the arcade controller's menu/wind/scoreboard behavior. The platform layer
 * feeds queued InputEvents and a frame dt, renders from this struct, and performs saves. */
#ifndef BYC_GAME_H
#define BYC_GAME_H

#include <stdbool.h>
#include <stdint.h>
#include "physics.h"
#include "rng.h"
#include "cpu.h"
#include "input.h"
#include "save.h"
#include "music.h"
#include "arrow.h"

#ifndef BYC_VERSION
#define BYC_VERSION "0.1.5"
#endif

#define GAME_MENU_MAX 16

typedef enum { PH_MENU, PH_INTRO, PH_CHAMPION, PH_WAIT, PH_AIM, PH_CPU, PH_FLYING, PH_SETTLE, PH_SUMMARY, PH_OVER } Phase;
typedef enum {
    UI_NONE, UI_STRAP, UI_TITLE, UI_SETTINGS, UI_SETPOWER, UI_CAREER_MAP, UI_INTRO, UI_CHAMPION,
    UI_SUMMARY, UI_CAREER_RESULT, UI_PAUSE, UI_CONFIRM_QUIT, UI_CONFIRM_NEW_CAREER, UI_MUSIC
} UiScreen;
typedef enum {
    MI_RESUME, MI_PRACTICE, MI_TWO_PLAYER, MI_TOURNAMENT, MI_SETTINGS, MI_QUIT, MI_CLEAR_BOARD, MI_MAIN_MENU,
    MI_HAND_RED, MI_HAND_BLUE, MI_POWER_RED, MI_POWER_BLUE, MI_SETPOWER_RED, MI_SETPOWER_BLUE,
    MI_CURL_RED, MI_CURL_BLUE, MI_WIND, MI_RESET_SETTINGS, MI_BACK, MI_CONTINUE, MI_NEW_CAREER,
    MI_LETS_TOSS, MI_CIRCUIT_MAP, MI_NEXT, MI_YES, MI_NO, MI_OK,
    MI_MUSIC, MI_MUSIC_TOGGLE, MI_MUSIC_NEXT, MI_MUSIC_FOLDER, MI_MUSIC_REFRESH, MI_MUSIC_VOLUME
} MenuItemId;
typedef struct { MenuItemId id; char label[24]; char value[16]; bool disabled; } MenuItem;
typedef struct { MenuItem items[GAME_MENU_MAX]; int count, sel; } Menu;

typedef enum { FX_HOLE_BURST, FX_DUST, FX_CELEBRATE, FX_DOG_RESET, FX_RUMBLE } FxKind;
typedef struct { FxKind kind; Vec3 at; int ms; unsigned channel; } Fx;

typedef enum { T_NONE, T_NEXT_TURN, T_HIDE_MSG, T_SAY_LATER } TimerKind;
typedef struct { TimerKind kind; double left_ms; char big[24], small[48]; int say_ms; } Timer;

typedef struct { int pts[2], in_c[2], on_c[2]; } Lie;
typedef struct { Lie lie; int net, scorer, inning, scores[2]; bool bust; } RoundResult;

typedef struct {
    /* match state (index.html M) */
    /* Bags grow on demand: practice keeps every bag until CLEAR BOARD (1.0.3), with no cap. Entries
     * that have left the world (in the hole, removed grounders) are compacted away between throws;
     * their points stay in `banked` so the practice readout never changes. */
    Bag *bags;
    int nbags, active;            /* active: index of the bag in flight or -1 */
    BagState *before;
    int nbefore, bag_cap;
    int bag_cap_limit;            /* test hook: > 0 simulates an allocation ceiling */
    Lie banked;
    Wind wind;
    Phase phase;
    bool paused;
    bool vs_cpu, career_match, practice, exact;
    int scores[2], inning, first, k, winner;
    double aim, spin;
    BagArc arc;
    int heights[2];
    BagSide side[2];
    double position[2];
    int practice_throws, practice_in, practice_on;
    CpuShot cpu;
    double cpu_t;
    bool cpu_planning;
    CpuPlan plan;
    long cpu_step_budget;
    RoundResult round;
    Lie live;                     /* scoreboard round points: recounted only when all bags rest */
    unsigned rival_color;

    /* career (localStorage backyard-cornhole-career-v1 semantics) */
    int fight, wins;

    /* HUD */
    char msg_big[24], msg_small[48];
    bool msg_on;
    /* The one combined direction + power arrow (arcade 1.0.7): live during the forward swing,
     * then held ARROW_HOLD_MS after release and faded. There is no separate power meter. */
    double arrow_yaw, arrow_power, arrow_x, arrow_age_ms;
    bool arrow_on, arrow_live;
    bool help_shown;
    bool pass_and_play;
    double meter;

    /* screens */
    UiScreen ui, settings_return, pause_return, music_return;
    Menu menu;
    int setpower_slot, setpower_count;
    char setpower_note[16];
    bool connect_prompt;
    unsigned connect_remote;
    bool connect_offer_reassign;
    bool resume_pending;
    bool save_ok;

    Timer timers[8];
    Fx fx[16];
    int fx_head, fx_count;

    SaveData save;
    bool save_dirty, quit_requested;
    double time, acc;
    uint64_t now_ms;
    Rng rng;
    const WinTable *win;
    Input *input;
    Music *music;                 /* optional; set by the platform after game_init */
} Game;

/* *g must be zero-initialised (static storage) or previously passed to game_init/game_free. */
void game_init(Game *g, Input *input, const WinTable *win, uint32_t seed, const SaveData *loaded, bool save_ok);
void game_free(Game *g);
void game_set_music(Game *g, Music *music);
void game_input(Game *g, const InputEvent *e);
void game_update(Game *g, double dt, uint64_t now_ms);
bool game_pop_fx(Game *g, Fx *out);

int game_thrower(const Game *g);
bool game_is_human(const Game *g, int team);
double game_release_x(const Game *g, int team);
const char *game_team_name(const Game *g, int team);
Lie game_lie(const Game *g);
bool game_menu_screen(const Game *g);     /* a panel/menu is open */
/* Provisional SET POWER limits (pending P1 hardware traces; see wii/INPUT.md). */
SwingCalibrationLimits game_calibration_limits(const Game *g);

/* test/automation hooks */
void game_start(Game *g, bool practice, bool vs_cpu, bool career);
bool game_throw(Game *g, double power, double aim, double spin);  /* as a human INPUT_THROW */
void game_menu_select(Game *g, MenuItemId id);
void game_clear_board(Game *g);
/* Launch yaw and physical power the HUD arrow shows for a toss (same maths as the throw). */
/* "v" BYC_VERSION: drawn on the strap screen and in a corner of the in-game HUD. */
const char *game_version_tag(void);
double game_launch_yaw(const Game *g, int team, double aim);

#endif
