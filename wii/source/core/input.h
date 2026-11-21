#ifndef BYC_INPUT_H
#define BYC_INPUT_H

#include "swing.h"

enum InputButton {
    INPUT_TWO=1u<<0, INPUT_ONE=1u<<1, INPUT_B=1u<<2, INPUT_A=1u<<3,
    INPUT_MINUS=1u<<4, INPUT_HOME=1u<<7, INPUT_LEFT=1u<<8, INPUT_RIGHT=1u<<9,
    INPUT_DOWN=1u<<10, INPUT_UP=1u<<11, INPUT_PLUS=1u<<12
};
typedef enum { INPUT_PLAY, INPUT_MENU, INPUT_DISABLED } InputContext;
typedef enum {
    INPUT_THROW, INPUT_HEIGHT, INPUT_SURFACE, INPUT_STEP, INPUT_RESET_BAG,
    INPUT_HELP, INPUT_PAUSE, INPUT_CONNECT_REMOTE, INPUT_CONFIRM, INPUT_BACK,
    INPUT_NAV_UP, INPUT_NAV_DOWN, INPUT_NAV_LEFT, INPUT_NAV_RIGHT,
    INPUT_ADJUST_MINUS, INPUT_ADJUST_PLUS,
    INPUT_CAL_MORE, INPUT_CAL_DONE, INPUT_CAL_RETRY, INPUT_PROFILE_CHANGED
} InputAction;
typedef struct {
    InputAction action;
    unsigned slot, channel; /* zero-based; CONNECT_REMOTE channel + 1 is the prompt */
    double value;
    SwingResult toss;
} InputEvent;
typedef void (*InputSink)(void *user, const InputEvent *event);
typedef struct {
    SwingSample motion;
    uint32_t buttons;
    bool buttons_valid;
} InputSample;
typedef struct {
    SwingProfile profile;
    double aim, stance, stance_from;
    uint64_t stance_ms;
    int height; /* 0 SLIDER, 1 NORMAL, 2 KOBE */
    bool sticky;
} InputSlot;
typedef struct {
    uint64_t window_ms, last_ms;
    unsigned window_count, rate, windows;      /* reports per second, from the last full 1 s window */
    unsigned gap_ms, window_gap_ms;   /* largest report gap in the last full 2 s window / this one */
    bool have_last, have_toss;
    SwingResult last_toss;
} InputStats;
typedef struct {
    InputSlot slots[2];
    Swing detector;
    bool connected[2], b_held[2], paused, disconnect_pause, help;
    uint32_t held[2];
    uint64_t debug_hold_ms[2];
    bool debug_holding[2], debug_latched[2], debug_visible;
    unsigned channels[2], active_slot;
    InputContext context;
    bool hand_option;
    unsigned hand_slot;
    bool calibrating;
    SwingCalibration calibration;
    SwingCalibrationLimits calibration_limits;
    InputStats stats; /* active remote's report timing + last throw, for the 1+2 overlay */
    InputSink sink;
    void *user;
} Input;

void input_init(Input *input, InputSink sink, void *user);
void input_connection(Input *input, unsigned channel, bool connected);
void input_report_loss(Input *input); /* packet timing/queue loss: discard gesture, preserve spent */
void input_start_match(Input *input); /* snapshots Remote 2 availability */
void input_next_turn(Input *input, unsigned slot); /* a B already held grips at once */
void input_set_context(Input *input, InputContext context);
bool input_resume(Input *input); /* reconnect/reassign AND B up required */
void input_feed(Input *input, unsigned channel, const InputSample *sample);
bool input_set_profile(Input *input, unsigned slot, SwingProfile profile);
bool input_begin_calibration(Input *input, SwingCalibrationLimits limits);
void input_cancel_calibration(Input *input);
unsigned input_largest_gap(const Input *input);
double input_stance(const Input *input, unsigned slot, uint64_t now_ms);
/* Setup/pause menu focuses HAND for an explicit player slot; left/right selects. */
bool input_focus_hand(Input *input, unsigned slot);
void input_blur_hand(Input *input);
extern const char input_help_text[];

#endif
