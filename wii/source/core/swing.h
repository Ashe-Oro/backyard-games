#ifndef BYC_SWING_H
#define BYC_SWING_H

#include <stdbool.h>
#include <stdint.h>
#include "swing_config.h"

typedef struct { double x, y, z; } SwingVector;
typedef struct {
    uint64_t ms;
    SwingVector g;
    bool b, clipped;
    bool accel_valid;
    unsigned dropped;    /* informational; a dropped report never resets the swing */
    bool manual_release; /* Bowling input uses B-up as the only release trigger. */
} SwingSample;
typedef struct { double full_speed, spin_strength; bool left_handed; } SwingProfile;
/* READY grips on the very report B reads down: no settle or stillness step, any pose. */
typedef enum { SWING_READY, SWING_GRIP, SWING_PENDING, SWING_SPENT } SwingPhase;
typedef enum { SWING_NO_RELEASE, SWING_B_UP, SWING_AUTO, SWING_TIMEOUT } SwingRelease;
typedef struct {
    bool thrown, fallback; /* fallback: weak throw (no classified forward stroke), never nothing */
    double power, speed, aim, spin, roll;
    bool clipped;
    SwingRelease release;
    uint64_t release_ms;
} SwingResult;
typedef struct { uint64_t ms; SwingVector g; double e; bool clipped; } SwingPoint;
typedef struct {
    SwingPhase phase;
    SwingPoint history[SWING_HISTORY];
    unsigned count;
    uint64_t last_ms, armed_ms, lobe_start, lobe_end, release_ms;
    bool have_time, have_g, last_clipped, previous_b, require_b_up;
    bool in_lobe, lobe_valid, lobe_clipped, confirmed, back_apex, dipping, roll_ref;
    unsigned lobes;                      /* strokes since the grip; the second and later are forward */
    unsigned bridged, restarts;          /* gaps interpolated / long gaps that restarted motion */
    SwingVector last_g;
    double mean_e, speed, peak, lobe_peak, lobe_peak_e, dip_e, locked_aim, locked_spin, locked_roll;
    double roll_x, roll_z;
    uint64_t roll_since;
    double lateral_sum, lateral_weight, motion_roll, fine_aim;
    SwingVector grip_g;
    uint64_t back_motion_ms, return_started_ms, return_ms;
    double back_pitch, previous_pitch, return_high_pitch, return_travel, return_peak;
    double return_lateral_sum;
} Swing;

SwingProfile swing_default_profile(void);
double swing_speed(double mean_excess_g);
double swing_power(double peak_speed, double full_speed);
double swing_roll_spin(double roll_degrees, const SwingProfile *profile);
/* Computes an exactly clipped, trapezoidal 80 ms window. Does not integrate distance. */
double swing_window_mean(const SwingPoint *points, unsigned count, uint64_t now, unsigned *span);
/* Trace only: call after release+133 ms, using a retained raw-sample history. Never drives curl. */
double swing_twist_relative(const SwingPoint *points, unsigned count, uint64_t release_ms);
/* require_b_up: a B already held must be let go before it can grip (pause, menus, buttons). */
void swing_reset(Swing *s, bool require_b_up);
bool swing_armed(const Swing *s);
bool swing_forward_motion(const Swing *s);
/* READY, GRIP, BACKSWING, FORWARD, RELEASE or SPENT, for the debug overlay. */
const char *swing_phase_name(const Swing *s);
/* Read-only HUD preview of the classified forward lobe (after the back-apex pause, before the
   toss is emitted): the power and aim a release right now would lock. Never changes the throw. */
bool swing_preview(const Swing *s, const SwingProfile *profile, double *power, double *aim);
SwingResult swing_feed(Swing *s, const SwingSample *sample, double aim, const SwingProfile *profile);

/* Supply P_TARGET from physics, and floor/ceiling from P1 measured rails. No invented defaults. */
typedef struct { double target_power, full_speed_floor, clip_ceiling; } SwingCalibrationLimits;
typedef struct { double peaks[SWING_CAL_TOSSES]; unsigned count; } SwingCalibration;
typedef enum { SWING_CAL_MORE, SWING_CAL_DONE, SWING_CAL_RETRY, SWING_CAL_BAD_LIMITS } SwingCalibrationStatus;
SwingCalibrationStatus swing_calibrate(SwingCalibration *cal, const SwingResult *toss,
                                      SwingCalibrationLimits limits, SwingProfile *profile);

#endif
