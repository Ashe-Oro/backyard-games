#ifndef BYC_SWING_CONFIG_H
#define BYC_SWING_CONFIG_H

/* Candidate constants, not hardware-validated tuning. Units are ms, g, m/s, degrees. */
#define SWING_VERSION "bowling-grip-0.1.7"
#define SWING_WINDOW_MS 80
#define SWING_MIN_SPAN_MS 16
#define SWING_HISTORY_MS 150
#define SWING_GRIP_MS 100
#define SWING_DIRECTION_FILTER_MS 6
#define SWING_GRIP_LOAD_G 1.20
#define SWING_BACK_TANGENT_G 0.115
#define SWING_REVERSAL_DROP_G 0.09
#define SWING_REVERSAL_RISE_G 0.035
#define SWING_DIRECTION_MS 10
#define SWING_FORWARD_G 0.22
#define SWING_FORWARD_MS 24
#define SWING_RELEASE_MS 140
#define SWING_TIMEOUT_MS 2500
#define SWING_AUTO_RATIO 0.6
#define SWING_GAP_MS 25          /* report spacing above this is a gap */
#define SWING_BRIDGE_MS 100      /* gaps up to this are bridged by interpolated samples */
#define SWING_BRIDGE_STEP_MS 10
#define SWING_RESET_GAP_MS 250   /* longer gaps restart the motion measurement; a held grip is kept */
#define SWING_ROLL_LOAD_G 0.35
#define SWING_ROLL_REF_G 0.5     /* grip roll needs this much gravity in the remote's X-Z plane */
#define SWING_ROLL_REF_MS 60     /* otherwise the roll reference is the first 60 ms of loaded swing */
#define SWING_RADIUS_M 0.6
#define SWING_GRAVITY 9.80665
#define SWING_DEFAULT_E_G 1.9
#define SWING_POWER_GAIN 0.55
#define SWING_POWER_CAP 0.76    /* calm, centered NORMAL lands 8.90 m from release */
#define SWING_CAL_TOSSES 3
#define SWING_CAL_SPREAD 0.30
#define SWING_ROLL_DEAD_DEG 10.0
#define SWING_ROLL_FULL_DEG 45.0
#define SWING_AIM_MAX 0.12
#define SWING_LATERAL_FULL 0.5
#define SWING_LATERAL_DEAD 0.03
#define SWING_STEP 0.05
#define SWING_STEP_CLAMP 0.45
#define SWING_STEP_EASE_MS 70
#define SWING_FINISH_DEG 15.0
#define SWING_STABLE_G 0.10
#define SWING_TWIST_MS 133
#define SWING_HISTORY 192
#define SWING_PI 3.14159265358979323846

#endif
