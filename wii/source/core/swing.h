#ifndef BYC_SWING_H
#define BYC_SWING_H

#include <stdbool.h>
#include <stdint.h>
#include "swing_config.h"

typedef struct { double x, y, z; } SwingVector;
typedef struct {
    uint64_t ms;
    SwingVector g;
    bool b, ir, clipped;
    bool accel_valid;
    unsigned dropped;
} SwingSample;
typedef struct { double full_speed, spin_strength; bool left_handed; } SwingProfile;
typedef enum { SWING_SETTLING, SWING_READY, SWING_GRIP, SWING_PENDING, SWING_SPENT } SwingPhase;
typedef enum { SWING_NO_RELEASE, SWING_B_UP, SWING_AUTO, SWING_TIMEOUT } SwingRelease;
typedef struct {
    bool thrown, cancelled, hold_still;
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
    uint64_t last_ms, quiet_since, armed_ms, lobe_start, lobe_end, release_ms;
    bool have_time, quiet, previous_b, require_b_up;
    bool in_lobe, lobe_valid, lobe_clipped, lost_ir, saw_back, confirmed;
    double mean_e, speed, peak, lobe_peak, locked_aim, locked_spin, locked_roll;
    SwingVector grip_g;
} Swing;

SwingProfile swing_default_profile(void);
double swing_speed(double mean_excess_g);
double swing_power(double peak_speed, double full_speed);
double swing_roll_spin(double roll_degrees, const SwingProfile *profile);
/* Computes an exactly clipped, trapezoidal 80 ms window. Does not integrate distance. */
double swing_window_mean(const SwingPoint *points, unsigned count, uint64_t now, unsigned *span);
/* Trace only: call after release+133 ms, using a retained raw-sample history. Never drives curl. */
double swing_twist_relative(const SwingPoint *points, unsigned count, uint64_t release_ms);
void swing_reset(Swing *s, bool require_b_up);
bool swing_armed(const Swing *s);
bool swing_calm_roll(const Swing *s, double *roll_degrees);
SwingResult swing_feed(Swing *s, const SwingSample *sample, double aim, const SwingProfile *profile);

/* Supply P_TARGET from physics, and floor/ceiling from P1 measured rails. No invented defaults. */
typedef struct { double target_power, full_speed_floor, clip_ceiling; } SwingCalibrationLimits;
typedef struct { double peaks[SWING_CAL_TOSSES]; unsigned count; } SwingCalibration;
typedef enum { SWING_CAL_MORE, SWING_CAL_DONE, SWING_CAL_RETRY, SWING_CAL_BAD_LIMITS } SwingCalibrationStatus;
SwingCalibrationStatus swing_calibrate(SwingCalibration *cal, const SwingResult *toss,
                                      SwingCalibrationLimits limits, SwingProfile *profile);

#endif
