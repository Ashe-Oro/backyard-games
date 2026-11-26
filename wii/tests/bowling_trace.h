/* Shoulder-pivot fixture from the swing research simulator. The remote reads
 * 1 g up at rest, with -Y along the tip and +Z out of the button face. */
#ifndef BYC_BOWLING_TRACE_H
#define BYC_BOWLING_TRACE_H

#include "swing.h"
#include <math.h>

typedef struct {
    double start_deg, back_deg, forward_deg;
    double back_s, forward_s, pause_s;
    double pitch_at_bottom, release_deg, release_s;
    double lateral_g, twist_deg, tremor_deg;
    bool inline_grip, rolled, back_only, still, held, release_going_back;
} BowlingTrace;

static inline BowlingTrace bowling_trace(void) {
    return (BowlingTrace){
        .start_deg=45, .back_deg=-50, .forward_deg=70,
        .back_s=.55, .forward_s=.45, .release_deg=15, .release_s=-1
    };
}

#define BOWLING_PRESS_S .50
#define BOWLING_START_S .80

static inline double bowling_forward(const BowlingTrace *t) {
    return BOWLING_START_S+t->back_s+t->pause_s;
}

static inline double bowling_release(const BowlingTrace *t) {
    if (t->held) return INFINITY;
    if (t->release_s>=0) return t->release_s;
    double from=t->release_going_back ? t->start_deg : t->back_deg;
    double to=t->release_going_back ? t->back_deg : t->forward_deg;
    double start=t->release_going_back ? BOWLING_START_S : bowling_forward(t);
    double duration=t->release_going_back ? t->back_s : t->forward_s;
    double fraction=(t->release_deg-from)/(to-from);
    return start+duration*acos(1-2*fmax(0,fmin(1,fraction)))/SWING_PI;
}

static inline double bowling_fraction(double u) {
    return fmax(0,fmin(1,u));
}

static inline double bowling_ease(double u) {
    return (1-cos(SWING_PI*bowling_fraction(u)))/2;
}

static inline double bowling_blend(double u) {
    u=bowling_fraction(u);
    return u*u*u*(10+u*(-15+6*u));
}

static inline double bowling_angle(const BowlingTrace *t, double s) {
    if (t->still) return t->start_deg;
    if (s<bowling_forward(t) || t->back_only)
        return t->start_deg+(t->back_deg-t->start_deg)*bowling_ease((s-BOWLING_START_S)/t->back_s);
    return t->back_deg+(t->forward_deg-t->back_deg)*bowling_ease((s-bowling_forward(t))/t->forward_s);
}

static inline void bowling_position(const BowlingTrace *t, double s, double *forward, double *up,
                                    double *tip) {
    double phi=bowling_angle(t,s)+t->tremor_deg*sin(2*SWING_PI*5*s);
    double bottom=acos(1-2*t->start_deg/(t->start_deg-t->back_deg))/SWING_PI;
    double elapsed=s-BOWLING_START_S;
    double wrist=t->still ? 0 : bowling_blend(elapsed/(1.3*bottom*t->back_s));
    double radius=.50+.12*wrist;
    double c=(90-t->start_deg)*(1-wrist)+t->pitch_at_bottom*wrist;
    if (t->inline_grip) {
        double span=1.3*bottom*t->back_s;
        if (span<.15) span=.35;
        c-=90*bowling_blend(elapsed/span);
    }
    double angle=phi*SWING_PI/180;
    *forward=radius*sin(angle);
    *up=-radius*cos(angle);
    *tip=(phi+c)*SWING_PI/180;
}

static inline SwingVector bowling_force(const BowlingTrace *t, double s) {
    double f0,u0,f1,u1,f2,u2,tip,unused;
    const double h=.001;
    bowling_position(t,s-h,&f0,&u0,&unused);
    bowling_position(t,s,&f1,&u1,&tip);
    bowling_position(t,s+h,&f2,&u2,&unused);
    double forward=(f2-2*f1+f0)/(h*h*SWING_GRAVITY);
    double up=1+(u2-2*u1+u0)/(h*h*SWING_GRAVITY);
    SwingVector g={0,-(forward*cos(tip)+up*sin(tip)),up*cos(tip)-forward*sin(tip)};
    double velocity=(f2-f0)/(2*h);
    g.x=t->lateral_g*bowling_fraction((velocity-.3)/2);
    if (t->twist_deg!=0) {
        double release=bowling_release(t);
        double twist=t->twist_deg*bowling_fraction((s-release+.12)/.12)*SWING_PI/180;
        double x=g.x*cos(twist)+g.z*sin(twist);
        g.z=g.z*cos(twist)-g.x*sin(twist);
        g.x=x;
    }
    if (t->rolled) {
        g.x=-g.x;
        g.z=-g.z;
    }
    return g;
}

static inline bool bowling_button(const BowlingTrace *t, double s) {
    return s>=BOWLING_PRESS_S && s<bowling_release(t);
}

static inline double bowling_uniform(uint32_t *seed) {
    *seed=*seed*1664525u+1013904223u;
    return (*seed>>8)/16777216.0;
}

static inline double bowling_noise(uint32_t *seed) {
    double sum=0;
    for (unsigned i=0;i<6;++i) sum+=bowling_uniform(seed);
    return (sum-3)/sqrt(.5);
}

/* 104 counts/g, modeled ±3.6 g sensor rail. BTNS_ACC omits the Y/Z LSB. */
static inline unsigned bowling_raw_axis(double g, bool tilt, uint32_t *seed) {
    double raw=round(512+104*(g+.015*bowling_noise(seed)));
    unsigned code=(unsigned)fmax(512-374,fmin(512+374,raw));
    return tilt ? (code & ~1u) : code;
}

static inline SwingVector bowling_quantized(SwingVector g, uint32_t *seed) {
    unsigned x=bowling_raw_axis(g.x,false,seed);
    unsigned y=bowling_raw_axis(g.y,true,seed);
    unsigned z=bowling_raw_axis(g.z,true,seed);
    return (SwingVector){(x-512.0)/104,(y-512.0)/104,(z-512.0)/104};
}
#endif
