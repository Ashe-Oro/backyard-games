/* Shared synthetic Bowling-swing traces for throw_test.c and wpad_test.c (Wii 0.1.3).
 * Shoulder-pivot arm model (remote Y tangent, Z radial), hand tremor, Gaussian sensor
 * noise and 10-bit quantization at 104 counts/g around 512. "Remote up" (start_deg 80) is
 * the arm raised forward so the remote's +Y axis reads gravity. Modeled, not captured. */
#ifndef BYC_BOWLING_TRACE_H
#define BYC_BOWLING_TRACE_H
#include "swing.h"
#include <math.h>

/* ---------- deterministic noise ---------- */
static uint32_t rng_state;
static inline double uniform(void) { rng_state=rng_state*1664525u+1013904223u; return (rng_state>>8)/16777216.0; }
static inline double gauss(void) { double s=0; for (int k=0;k<6;++k) s+=uniform(); return (s-3)/sqrt(.5); }

/* ---------- trace model ---------- */
typedef struct {
    unsigned rate_hz, jitter_ms;
    bool gaps;              /* 30-60 ms holes: at grip, in the backswing, at the bottom, across release */
    bool press_moving;      /* remote still travelling into the start pose when B goes down */
    bool tap;               /* B down and up with no swing */
    bool back_only;
    double start_deg, back_deg, fwd_deg, back_s, fwd_s, release_deg;
    double lateral_g, twist_deg;
    bool left;
    uint32_t seed;
} Trace;
typedef struct { uint64_t ms; double theta, omega, alpha; bool b; double lateral, roll; } Pose;

static inline double ease(double from, double to, double u, double dur, double *omega, double *alpha) {
    double d=to-from, k=SWING_PI/dur;
    *omega=d*k*sin(k*u)/2; *alpha=d*k*k*cos(k*u)/2;
    return from+d*(1-cos(k*u))/2;
}
#define T_PRESS 200
#define T_SWING 600
static inline double backswing_start_ms(void) { return T_SWING; }
static inline double forward_start_ms(const Trace *t) { return T_SWING + 1000*t->back_s; }
/* Kinematics at time ms (relative to the turn opening). Returns false once the trace is over. */
static inline bool pose(const Trace *t, double ms, Pose *p, uint64_t *release_ms) {
    double s=ms/1000, deg=SWING_PI/180;
    double start=t->start_deg*deg, back=-t->back_deg*deg, fwd=t->fwd_deg*deg;
    p->omega=p->alpha=0; p->lateral=0; p->roll=0;
    double tremor=1.5*deg*sin(2*SWING_PI*5*s);
    if (t->tap) {
        p->theta=start+tremor;
        p->b=ms>=T_PRESS && ms<T_PRESS+150;
        if (release_ms && ms>=T_PRESS+150 && !*release_ms) *release_ms=(uint64_t)ms;
        return ms<1400;
    }
    double t_back=backswing_start_ms()/1000, t_fwd=forward_start_ms(t)/1000, t_end=t_fwd+t->fwd_s;
    if (s<.4) {
        if (t->press_moving) p->theta=ease(20*deg,start,s,.4,&p->omega,&p->alpha);
        else p->theta=start+tremor;
    } else if (s<t_back) p->theta=start+tremor;
    else if (s<t_fwd) p->theta=ease(start,back,s-t_back,t->back_s,&p->omega,&p->alpha);
    else if (t->back_only) p->theta=back;
    else if (s<t_end) {
        p->theta=ease(back,fwd,s-t_fwd,t->fwd_s,&p->omega,&p->alpha);
        p->lateral=t->lateral_g*sin(SWING_PI*(s-t_fwd)/t->fwd_s);
    } else p->theta=fwd;
    bool forward=s>=t_fwd;
    if (forward && (p->theta>=t->release_deg*deg || (t->back_only && s>=t_fwd+.1)) && release_ms && !*release_ms) *release_ms=(uint64_t)ms;
    if (release_ms && *release_ms) p->roll=t->twist_deg;
    p->b=ms>=T_PRESS && !(release_ms && *release_ms);
    return s<t_end+.6;
}
static inline bool in_gap(double ms, uint64_t release_ms) {
    static const double holes[][2]={{180,225},{720,760},{1020,1080}};
    for (unsigned k=0;k<3;++k) if (ms>=holes[k][0] && ms<holes[k][1]) return true;
    /* The B-up report itself arrives 35 ms late. */
    return release_ms && ms>=(double)release_ms && ms<(double)release_ms+35;
}
static inline SwingVector specific_force(const Pose *p, bool left) {
    double y=SWING_RADIUS_M*p->alpha/SWING_GRAVITY+sin(p->theta);
    double z=SWING_RADIUS_M*p->omega*p->omega/SWING_GRAVITY+cos(p->theta);
    double x=(left ? -1 : 1)*p->lateral;
    double r=(left ? -1 : 1)*p->roll*SWING_PI/180;
    return (SwingVector){x*cos(r)+z*sin(r), y, z*cos(r)-x*sin(r)};
}
/* 10-bit raw reading. The ADXL330 saturates near +/-3.6 g, well inside the 0..1023 code
 * range, so a real remote clips silently: the WPAD adapter's provisional 0..1023 rails never
 * flag it. *clipped is only set if the code range itself is hit. */
static inline unsigned raw_axis(double g, bool *clipped) {
    double raw=round(512+104*(g+.015*gauss()));
    if (raw<512-374) raw=512-374;
    if (raw>512+374) raw=512+374;
    if (raw<=0 || raw>=1023) *clipped=true;
    return (unsigned)raw;
}
static inline unsigned raw_tilt_axis(double g, bool *clipped) {
    return raw_axis(g,clipped) & ~1u; /* Buttons + accelerometer reports omit the Y/Z LSB. */
}

static inline Trace bowling(void) {
    return (Trace){.rate_hz=100,.start_deg=80,.back_deg=55,.fwd_deg=60,.back_s=.55,.fwd_s=.45,
        .release_deg=10,.seed=7};
}
#endif
