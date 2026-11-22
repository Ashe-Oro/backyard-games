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
typedef enum { SWING_READY, SWING_GRIP, SWING_BACKSWING, SWING_FORWARD, SWING_PENDING, SWING_SPENT } SwingPhase;
typedef enum { SWING_NO_RELEASE, SWING_B_UP, SWING_AUTO, SWING_TIMEOUT } SwingRelease;
typedef struct {
    bool thrown;
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
    uint64_t last_ms, armed_ms, forward_ms, loaded_since, last_loaded;
    bool have_time, have_g, last_clipped, previous_b, require_b_up;
    bool loaded, qualified, forward_clipped, roll_ref;
    unsigned bridged, restarts;
    SwingVector last_g, grip_g;
    double mean_e, speed, peak, locked_aim, locked_spin, locked_roll;
    double roll_x, roll_z;
    uint64_t roll_since;
    double lateral_sum, lateral_weight, motion_roll;
    uint64_t direction_ms, direction_since;
    double tangent, radial, radial_high, radial_low;
    bool descending, grip_loaded;
} Swing;

SwingProfile swing_default_profile(void);
double swing_speed(double mean_excess_g);
/* Picker order is SLIDER, NORMAL, KOBE. Scale applies before the common cap. */
double swing_type_scale(int height);
double swing_power(double peak_speed, double full_speed);
double swing_roll_spin(double roll_degrees, const SwingProfile *profile);
/* Computes an exactly clipped, trapezoidal 80 ms window. Does not integrate distance. */
double swing_window_mean(const SwingPoint *points, unsigned count, uint64_t now, unsigned *span);
/* Trace only: call after release+133 ms, using a retained raw-sample history. Never drives curl. */
double swing_twist_relative(const SwingPoint *points, unsigned count, uint64_t release_ms);
/* require_b_up: a B already held must be let go before it can grip (pause, menus, buttons). */
void swing_reset(Swing *s, bool require_b_up);
bool swing_armed(const Swing *s);
/* READY, GRIP, BACKSWING, FORWARD, RELEASE or SPENT, for the debug overlay. */
const char *swing_phase_name(const Swing *s);
/* Read-only preview of release power and aim during a qualifying forward swing. */
bool swing_preview(const Swing *s, const SwingProfile *profile, int height, double *power, double *aim);
SwingResult swing_feed(Swing *s, const SwingSample *sample, const SwingProfile *profile, int height);

/* Supply P_TARGET from physics, and floor/ceiling from P1 measured rails. No invented defaults. */
typedef struct { double target_power, full_speed_floor, clip_ceiling; } SwingCalibrationLimits;
typedef struct { double peaks[SWING_CAL_TOSSES]; unsigned count; } SwingCalibration;
typedef enum { SWING_CAL_MORE, SWING_CAL_DONE, SWING_CAL_RETRY, SWING_CAL_BAD_LIMITS } SwingCalibrationStatus;
SwingCalibrationStatus swing_calibrate(SwingCalibration *cal, const SwingResult *toss,
                                      SwingCalibrationLimits limits, SwingProfile *profile);

#endif
