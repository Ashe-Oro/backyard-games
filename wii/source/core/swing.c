#include "swing.h"
#include <math.h>
#include <string.h>

static double clamp(double x, double low, double high) { return fmax(low, fmin(high, x)); }
static double norm(SwingVector v) { return sqrt(v.x*v.x + v.y*v.y + v.z*v.z); }
static double angle(SwingVector a, SwingVector b) {
    double scale = norm(a)*norm(b);
    return scale > 0 ? acos(clamp((a.x*b.x+a.y*b.y+a.z*b.z)/scale, -1, 1))*180/SWING_PI : 180;
}
static double pitch(SwingVector g) { return atan2(g.y, g.z)*180/SWING_PI; }
static double delta_degrees(double a, double b) { return remainder(a-b, 360); }

SwingProfile swing_default_profile(void) {
    return (SwingProfile){swing_speed(SWING_DEFAULT_E_G), 1, false};
}
double swing_speed(double e) {
    return isfinite(e) && e > 0 ? sqrt(SWING_RADIUS_M*SWING_GRAVITY*e) : 0;
}
double swing_power(double peak, double full) {
    return isfinite(peak) && isfinite(full) && full > 0 ? clamp(peak/full, 0, 1) : 0;
}
double swing_roll_spin(double roll, const SwingProfile *p) {
    if (!isfinite(roll) || !isfinite(p->spin_strength)) return 0;
    double strength = clamp((fabs(roll)-SWING_ROLL_DEAD_DEG)/
                            (SWING_ROLL_FULL_DEG-SWING_ROLL_DEAD_DEG), 0, 1);
    return -copysign(strength, roll)*clamp(p->spin_strength, 0, 1)*(p->left_handed ? -1 : 1);
}
double swing_window_mean(const SwingPoint *p, unsigned n, uint64_t now, unsigned *span) {
    double area = 0;
    uint64_t start = now > SWING_WINDOW_MS ? now-SWING_WINDOW_MS : 0;
    uint64_t first = now;
    for (unsigned i = 1; i < n; ++i) {
        uint64_t a = p[i-1].ms, b = p[i].ms;
        if (b <= start || a >= now || b <= a) continue;
        uint64_t lo = a < start ? start : a, hi = b > now ? now : b;
        /* Saturation is excluded; it can only cap an independently valid lobe. */
        double ea = p[i-1].clipped ? 0 : p[i-1].e;
        double eb = p[i].clipped ? 0 : p[i].e;
        double x = ea+(eb-ea)*(double)(lo-a)/(double)(b-a);
        double y = ea+(eb-ea)*(double)(hi-a)/(double)(b-a);
        area += (x+y)*0.5*(double)(hi-lo);
        if (lo < first) first = lo;
    }
    *span = (unsigned)(now-first);
    return *span ? area/(*span) : 0;
}
double swing_twist_relative(const SwingPoint *p, unsigned n, uint64_t release_ms) {
    double top[4]={-INFINITY,-INFINITY,-INFINITY,-INFINITY};
    for (unsigned k=0;k<n;++k) {
        uint64_t distance=p[k].ms>release_ms ? p[k].ms-release_ms : release_ms-p[k].ms;
        if (distance>SWING_TWIST_MS || !isfinite(p[k].g.x)) continue;
        double value=p[k].g.x;
        for (unsigned j=0;j<4;++j) if (value>top[j]) {
            double previous=top[j]; top[j]=value; value=previous;
        }
    }
    double sum=0;
    for (unsigned j=0;j<4;++j) if (isfinite(top[j])) sum+=top[j];
    return sum;
}
void swing_reset(Swing *s, bool up) {
    memset(s, 0, sizeof(*s));
    s->phase = SWING_SETTLING;
    s->require_b_up = up;
}
bool swing_armed(const Swing *s) { return s->phase == SWING_GRIP || s->phase == SWING_PENDING; }

static bool stable(const Swing *s, SwingVector *mean) {
    if (!s->count) return false;
    uint64_t now = s->history[s->count-1].ms;
    unsigned first = 0;
    while (first+1 < s->count && now-s->history[first+1].ms >= SWING_STABLE_MS) ++first;
    if (now-s->history[first].ms < SWING_STABLE_MS) return false;
    *mean = (SwingVector){0,0,0};
    for (unsigned i = first; i < s->count; ++i) {
        if (s->history[i].clipped || s->history[i].e >= SWING_STABLE_G) return false;
        for (unsigned j = first; j < i; ++j)
            if (angle(s->history[i].g, s->history[j].g) >= SWING_STABLE_DEG) return false;
        mean->x += s->history[i].g.x;
        mean->y += s->history[i].g.y;
        mean->z += s->history[i].g.z;
    }
    return norm(*mean) > 0;
}
bool swing_calm_roll(const Swing *s, double *roll) {
    SwingVector mean;
    if (!stable(s,&mean)) return false;
    *roll=atan2(mean.x,mean.z)*180/SWING_PI;
    return true;
}
static SwingResult finish(Swing *s, const SwingProfile *profile, SwingRelease cause) {
    SwingResult r = {0};
    uint64_t reference_ms=cause==SWING_B_UP ? s->release_ms : s->last_ms;
    if (s->lobe_valid && s->confirmed && s->peak > 0 && reference_ms>=s->lobe_end &&
        reference_ms-s->lobe_end <= SWING_RELEASE_MS) {
        r = (SwingResult){.thrown=true, .speed=s->peak,
            .power=s->lobe_clipped ? 1 : swing_power(s->peak, profile->full_speed),
            .aim=s->locked_aim, .spin=s->locked_spin, .roll=s->motion_roll,
            .clipped=s->lobe_clipped, .release=cause,
            .release_ms=cause==SWING_B_UP ? s->release_ms : s->last_ms};
        s->phase = SWING_SPENT;
    } else {
        r.cancelled = true;
        swing_reset(s, true);
    }
    return r;
}
SwingResult swing_feed(Swing *s, const SwingSample *p, double aim, const SwingProfile *profile) {
    SwingResult r = {0};
    if (s->phase == SWING_SPENT) return r;
    bool armed = swing_armed(s);
    double mag = norm(p->g);
    if (!p->accel_valid || !isfinite(mag) || mag < 0.1 || p->dropped ||
        (s->have_time && (p->ms <= s->last_ms || p->ms-s->last_ms > SWING_GAP_MS))) {
        r.cancelled = armed;
        swing_reset(s, true);
        return r;
    }
    s->have_time = true;
    s->last_ms = p->ms;
    bool down = p->b && !s->previous_b, up = !p->b && s->previous_b;
    s->previous_b = p->b;
    if (!p->b) s->require_b_up = false;
    double e = fmax(0, mag-1);
    while (s->count > 1 && p->ms-s->history[1].ms > SWING_STABLE_MS) {
        memmove(s->history, s->history+1, (--s->count)*sizeof(s->history[0]));
    }
    if (s->count == SWING_HISTORY) {
        r.cancelled = armed;
        swing_reset(s, true);
        return r;
    }
    s->history[s->count++] = (SwingPoint){p->ms, p->g, e, p->clipped};
    unsigned span;
    s->mean_e = swing_window_mean(s->history, s->count, p->ms, &span);
    s->speed = span >= SWING_MIN_SPAN_MS && s->mean_e >= SWING_NOISE_G ? swing_speed(s->mean_e) : 0;
    if (s->phase == SWING_SETTLING) {
        if (s->mean_e < SWING_NOISE_G && e < SWING_NOISE_G && !p->clipped && !p->b) {
            if (!s->quiet) { s->quiet = true; s->quiet_since = p->ms; }
            if (p->ms-s->quiet_since >= SWING_QUIET_MS) s->phase = SWING_READY;
        } else s->quiet = false;
        return r;
    }
    if (s->phase == SWING_READY) {
        if (down && !s->require_b_up) {
            SwingVector mean;
            if (!stable(s, &mean)) { r.hold_still = true; return r; }
            s->fine_aim = s->locked_aim = clamp(aim, -SWING_AIM_MAX, SWING_AIM_MAX);
            s->locked_roll = atan2(mean.x, mean.z)*180/SWING_PI;
            s->locked_spin = 0;
            s->motion_roll = 0;
            s->grip_g = mean;
            s->armed_ms = p->ms;
            s->phase = SWING_GRIP;
        }
        return r;
    }
    /* C1: keep the last lobe, never the maximum of backswing + forward lobes. */
    if (s->phase != SWING_PENDING && !p->clipped && s->mean_e >= SWING_FORWARD_G && span >= SWING_MIN_SPAN_MS) {
        if (!s->in_lobe) {
            s->in_lobe = true;
            s->lobe_start = p->ms;
            s->lobe_peak = 0;
            s->lateral_sum = s->lateral_weight = 0;
            s->lobe_valid = s->lobe_clipped = s->confirmed = false;
            s->peak = 0;
        }
        /* Grip-relative lateral acceleration is a candidate swing-yaw proxy.
           Weight by excess force, never use this signal for power. */
        if (e >= SWING_FORWARD_G) {
            double lateral=p->g.x/mag-s->grip_g.x/norm(s->grip_g);
            double dt=(double)(p->ms-s->history[s->count-2].ms);
            s->lateral_sum+=lateral*e*dt;
            s->lateral_weight+=e*dt;
        }
        s->lobe_peak = fmax(s->lobe_peak, s->speed);
        s->lobe_end = p->ms;
        if (p->ms-s->lobe_start >= SWING_LOBE_MS) {
            s->lobe_valid = true;
            s->peak = s->lobe_peak;
        }
    } else if (!p->clipped && s->mean_e < SWING_FORWARD_G) s->in_lobe = false;
    if (p->clipped && s->lobe_valid && s->in_lobe) s->lobe_clipped = true;

    /* Freeze motion controls at B-up (or the last auto-release report).
       Post-release follow-through may confirm the toss, but cannot retarget it. */
    if (s->phase!=SWING_PENDING && !p->clipped) {
        double lateral=s->lateral_weight>0 ? s->lateral_sum/s->lateral_weight : 0;
        if (fabs(lateral)<SWING_LATERAL_DEAD) lateral=0;
        double yaw=clamp(lateral/SWING_LATERAL_FULL,-1,1);
        double hand=profile->left_handed ? -1 : 1;
        s->locked_aim=clamp(s->fine_aim+hand*yaw*SWING_AIM_MAX,-SWING_AIM_MAX,SWING_AIM_MAX);
        s->motion_roll=delta_degrees(atan2(p->g.x,p->g.z)*180/SWING_PI,s->locked_roll);
        s->locked_spin=clamp(swing_roll_spin(s->motion_roll,profile)-hand*yaw*profile->spin_strength,-1,1);
    }
    /* Calm forward-up finish confirms a new forward lobe. No external tracking. */
    if (e < SWING_STABLE_G && s->mean_e < SWING_NOISE_G && !p->clipped) {
        double dp = delta_degrees(pitch(p->g), pitch(s->grip_g));
        if (dp <= -SWING_FINISH_DEG) {
            s->confirmed = s->lobe_valid = s->lobe_clipped = false;
            s->peak = 0;
        } else if (s->lobe_valid && dp >= SWING_FINISH_DEG) s->confirmed = true;
    }
    if (up && s->phase == SWING_GRIP) {
        if (!s->lobe_valid) return finish(s, profile, SWING_B_UP);
        s->phase = SWING_PENDING;
        s->release_ms = p->ms;
    }
    if (s->phase == SWING_PENDING) {
        if (s->confirmed || p->ms-s->release_ms >= SWING_RELEASE_MS) return finish(s, profile, SWING_B_UP);
    } else if (s->lobe_valid && s->confirmed && p->ms-s->lobe_start >= SWING_WINDOW_MS &&
               s->speed <= SWING_AUTO_RATIO*s->peak) return finish(s, profile, SWING_AUTO);
    if (p->ms-s->armed_ms >= SWING_TIMEOUT_MS) return finish(s, profile, SWING_TIMEOUT);
    return r;
}

SwingCalibrationStatus swing_calibrate(SwingCalibration *cal, const SwingResult *toss,
                                      SwingCalibrationLimits lim, SwingProfile *profile) {
    if (!isfinite(lim.target_power) || lim.target_power <= 0 || lim.target_power > 1 ||
        !isfinite(lim.full_speed_floor) || lim.full_speed_floor <= 0 ||
        !isfinite(lim.clip_ceiling) || lim.clip_ceiling < lim.full_speed_floor)
        return SWING_CAL_BAD_LIMITS;
    if (!toss->thrown || toss->clipped || !isfinite(toss->speed) || toss->speed <= 0) {
        cal->count = 0;
        return SWING_CAL_RETRY;
    }
    if (cal->count >= SWING_CAL_TOSSES) cal->count = 0;
    cal->peaks[cal->count++] = toss->speed;
    if (cal->count < SWING_CAL_TOSSES) return SWING_CAL_MORE;
    double a=cal->peaks[0], b=cal->peaks[1], c=cal->peaks[2];
    double median = fmax(fmin(a,b), fmin(fmax(a,b),c));
    cal->count = 0;
    for (unsigned i=0; i<SWING_CAL_TOSSES; ++i)
        if (fabs(cal->peaks[i]-median) > SWING_CAL_SPREAD*median) return SWING_CAL_RETRY;
    profile->full_speed = clamp(median/lim.target_power, lim.full_speed_floor, lim.clip_ceiling);
    return SWING_CAL_DONE;
}
