#ifndef BYC_SWING_H
#define BYC_SWING_H

#include <stdbool.h>
#include <stdint.h>
#include "swing_config.h"

/* Raw libogc gforce. +X points left, +Y toward the expansion port, +Z out
   of the button face. Tip up reads (0,-1,0), flat buttons up reads (0,0,+1). */
typedef struct { double x, y, z; } SwingVector;
typedef struct {
    uint64_t ms;
    SwingVector g;
    bool b, clipped;
    bool accel_valid;
    unsigned dropped;    /* informational; the input adapter can keep a partial batch */
} SwingSample;
typedef struct { double full_speed, spin_strength; bool left_handed; } SwingProfile;
typedef enum {
    SWING_READY, SWING_GRIP, SWING_BACKSWING, SWING_TURN, SWING_FORWARD,
    SWING_PAST_BOTTOM, SWING_RESOLVE, SWING_SPENT
} SwingPhase;
typedef enum { SWING_NO_RELEASE, SWING_B_UP } SwingRelease;
typedef enum {
    SWING_REJECT_NONE, SWING_REJECT_NO_SWING, SWING_REJECT_BACKSWING,
    SWING_REJECT_BEFORE_BOTTOM, SWING_REJECT_TOO_WEAK, SWING_REJECT_TOO_LATE,
    SWING_REJECT_REPORT_GAP, SWING_REJECT_COUNT
} SwingReject;
typedef enum { SWING_SIGN_UNKNOWN, SWING_SIGN_GRIP, SWING_SIGN_TURN } SwingSignSource;
typedef struct {
    bool thrown;
    double power, speed, aim, spin, roll;
    bool clipped;
    SwingRelease release;
    SwingReject reject;
    uint64_t release_ms;
} SwingResult;
typedef struct {
    uint64_t ms;
    SwingVector g;
    double radial;
    bool clipped;
} SwingPoint;
typedef struct {
    SwingPhase phase;
    SwingReject last_reject;
    unsigned rejects[SWING_REJECT_COUNT];
    unsigned bridged, restarts;
    uint64_t last_ms, phase_ms, still_ms;
    bool have_time, have_g, last_clipped, previous_b, require_b_up;
    SwingVector last_g, static_g;
    double pitch, face_pitch, roll;
    bool pitch_static;

    /* Per-grip state starts here. */
    uint64_t release_ms, onset_ms, bottom_ms;
    unsigned passes;
    bool onset, have_axis, have_bottom, window_closed, observer_valid, unstable_axis;
    SwingVector grip_g;
    double grip_pitch, grip_turn;
    double ny, nz, radial, tangent, excess;
    int tangent_sign;
    SwingSignSource sign_source;
    double arm_angle, speed;

    /* First lobe, then the radial minimum/rise that marks the turnaround. */
    bool in_lobe;
    uint64_t lobe_start_ms, lobe_peak_ms;
    double lobe_peak, lobe_ny, lobe_nz, top_y, top_z;
    double turn_min, turn_radial, turn_tangent;
    unsigned turn_rise;

    /* Reused for the forward lobe. Emitted power and controls are sampled
       through B-up, while RESOLVE may still need the lobe shape. */
    SwingPoint history[SWING_HISTORY];
    unsigned count, first;
    double forward_peak, peak, fitted_speed, lateral_sum, lateral_weight, release_roll;
    bool forward_clipped, weak_crest;
} Swing;

SwingProfile swing_default_profile(void);
double swing_speed(double mean_excess_g);
/* Picker order is SLIDER, NORMAL, KOBE. Scale applies before the common cap. */
double swing_type_scale(int height);
double swing_power(double peak_speed, double full_speed);
double swing_full_speed_for_power(double peak_speed, double power, int height);
double swing_roll_spin(double roll_degrees, const SwingProfile *profile);
/* require_b_up: a B already held must be let go before it can grip (pause, menus, buttons). */
void swing_reset(Swing *s, bool require_b_up);
/* Cancel a grip on an initialized detector, retaining debug diagnostics. */
void swing_cancel(Swing *s, bool require_b_up);
void swing_report_gap(Swing *s);
void swing_expire(Swing *s, uint64_t now);
bool swing_armed(const Swing *s);
const char *swing_phase_name(const Swing *s);
const char *swing_reject_name(SwingReject reason);
/* Read-only preview of release power and aim during a qualifying forward swing. */
bool swing_preview(const Swing *s, const SwingProfile *profile, int height, double *power, double *aim);
SwingResult swing_feed(Swing *s, const SwingSample *sample, const SwingProfile *profile, int height);

typedef struct { double target_power, full_speed_floor, clip_ceiling; } SwingCalibrationLimits;
typedef struct { double peaks[SWING_CAL_TOSSES]; unsigned count; } SwingCalibration;
typedef enum { SWING_CAL_MORE, SWING_CAL_DONE, SWING_CAL_RETRY, SWING_CAL_BAD_LIMITS } SwingCalibrationStatus;
SwingCalibrationStatus swing_calibrate(SwingCalibration *cal, const SwingResult *toss,
                                      SwingCalibrationLimits limits, SwingProfile *profile);

#endif
