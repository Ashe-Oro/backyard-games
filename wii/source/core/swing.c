#include "swing.h"
#include <math.h>
#include <string.h>

static double clamp(double x, double low, double high) { return fmax(low, fmin(high, x)); }
static double norm(SwingVector v) { return sqrt(v.x*v.x + v.y*v.y + v.z*v.z); }
static double pitch(SwingVector g) { return atan2(g.y, g.z)*180/SWING_PI; }
static double delta_degrees(double a, double b) { return remainder(a-b, 360); }

SwingProfile swing_default_profile(void) {
    return (SwingProfile){swing_speed(SWING_DEFAULT_E_G), 1, false};
}
double swing_speed(double e) {
    return isfinite(e) && e > 0 ? sqrt(SWING_RADIUS_M*SWING_GRAVITY*e) : 0;
}
double swing_type_scale(int height) {
    if (height == 0) return 1.30;
    if (height == 2) return 1.0/0.90;
    return 1.20;
}
double swing_power(double peak, double full) {
    return isfinite(peak) && isfinite(full) && full > 0 ? clamp(SWING_POWER_GAIN * (peak/full), 0, SWING_POWER_CAP) : 0;
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
        /* A saturated report contributes no measurable excess. */
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
    s->phase = SWING_READY;
    s->require_b_up = up;
}
bool swing_armed(const Swing *s) {
    return s->phase != SWING_READY && s->phase != SWING_SPENT;
}
static double throw_power(const Swing *s, const SwingProfile *profile, int height) {
    return swing_power(s->peak, profile->full_speed * swing_type_scale(height));
}
bool swing_preview(const Swing *s, const SwingProfile *profile, int height, double *power, double *aim) {
    if (s->phase != SWING_FORWARD || !s->qualified || !s->loaded ||
        s->last_ms - s->last_loaded > SWING_RELEASE_MS) return false;
    *power = throw_power(s, profile, height);
    *aim = s->locked_aim;
    return true;
}
static double swing_yaw(double lateral) {
    return fabs(lateral)<SWING_LATERAL_DEAD ? 0 : clamp(lateral/SWING_LATERAL_FULL,-1,1);
}
const char *swing_phase_name(const Swing *s) {
    switch (s->phase) {
    case SWING_READY: return "READY";
    case SWING_GRIP: return "GRIP";
    case SWING_BACKSWING: return "BACKSWING";
    case SWING_FORWARD: return "FORWARD";
    case SWING_PENDING: return "RELEASE";
    case SWING_SPENT: return "SPENT";
    }
    return "READY";
}

/* ---------- history ---------- */
static void push_point(Swing *s, uint64_t ms, SwingVector g, double e, bool clipped) {
    while (s->count > 1 && ms-s->history[1].ms > SWING_HISTORY_MS) {
        memmove(s->history, s->history+1, (--s->count)*sizeof(s->history[0]));
    }
    /* Bursts can outrun the ring: drop the oldest sample, never the swing. */
    if (s->count == SWING_HISTORY) memmove(s->history, s->history+1, (--s->count)*sizeof(s->history[0]));
    s->history[s->count++] = (SwingPoint){ms, g, e, clipped};
}
/* Whatever the remote is doing when B goes down: mean acceleration over the last 100 ms
   (the B-down report included), unclipped if possible. No stillness test. */
static SwingVector grip_reference(const Swing *s) {
    SwingVector sum = {0,0,0};
    if (!s->count) return s->have_g ? s->last_g : (SwingVector){0,1,0};
    uint64_t now = s->history[s->count-1].ms;
    for (int pass = 0; pass < 2; ++pass) {
        for (unsigned i = 0; i < s->count; ++i) {
            if (now-s->history[i].ms > SWING_GRIP_MS || (pass == 0 && s->history[i].clipped)) continue;
            sum.x += s->history[i].g.x; sum.y += s->history[i].g.y; sum.z += s->history[i].g.z;
        }
        if (norm(sum) > 0) return sum;
    }
    return s->history[s->count-1].g;
}
/* ---------- grip ---------- */
static void grip(Swing *s) {
    s->locked_aim = 0; /* straight until the sideways swing says otherwise */
    s->grip_g = grip_reference(s);
    /* Roll is measurable from gravity only when the remote is not pointing straight up or down
       (Bowling's remote-up start is exactly that case). Otherwise lock it from the first loaded
       part of the swing, where the arm's radial force fills the X-Z plane. */
    double xz = sqrt(s->grip_g.x*s->grip_g.x + s->grip_g.z*s->grip_g.z);
    s->roll_ref = xz >= SWING_ROLL_REF_G*norm(s->grip_g);
    s->locked_roll = s->roll_ref ? atan2(s->grip_g.x, s->grip_g.z)*180/SWING_PI : 0;
    s->roll_x = s->roll_z = 0;
    s->roll_since = 0;
    s->locked_spin = 0;
    s->motion_roll = 0;
    s->armed_ms = s->last_ms;
    s->direction_ms = s->last_ms;
    s->direction_since = 0;
    s->tangent = s->grip_g.y / fmax(norm(s->grip_g), 0.1);
    s->radial = sqrt(fmax(0, 1-s->tangent*s->tangent));
    double current_radial = hypot(s->last_g.x, s->last_g.z);
    s->grip_loaded = current_radial > SWING_GRIP_LOAD_G;
    if (s->grip_loaded) s->radial = current_radial;
    s->radial_high = s->radial_low = s->radial;
    s->descending = false;
    s->forward_ms = s->loaded_since = s->last_loaded = 0;
    s->loaded = s->qualified = s->forward_clipped = false;
    s->mean_e = s->speed = s->peak = 0;
    s->lateral_sum = s->lateral_weight = 0;
    s->phase = SWING_GRIP;
}

static void restart_motion(Swing *s) {
    uint64_t armed_ms = s->armed_ms;
    grip(s);
    s->armed_ms = armed_ms;
}

/* Y is tangential and the roll-invariant XZ magnitude is mostly radial in the
   shoulder-pivot model. Y can change sign while braking. Use it relative to
   grip to find the start, and a radial dip and recovery to find the turn. */
static void track_direction(Swing *s, const SwingSample *p) {
    if (s->phase == SWING_FORWARD) return;
    uint64_t dt = p->ms - s->direction_ms;
    if (dt > SWING_GAP_MS) s->direction_since = 0;
    s->direction_ms = p->ms;
    double alpha = (double)dt / (SWING_DIRECTION_FILTER_MS + (double)dt);
    s->tangent += alpha * (p->g.y - s->tangent);
    s->radial += alpha * (hypot(p->g.x, p->g.z) - s->radial);
    if (s->phase == SWING_GRIP) {
        double grip_y = s->grip_g.y / fmax(norm(s->grip_g), 0.1);
        /* A grip taken on a loaded approach can see a large negative braking
           force before the remote reaches its start pose. A falling radial
           load alone cannot start the backswing in that case. */
        if (s->grip_loaded) {
            s->radial_low = fmin(s->radial_low, s->radial);
            if ((s->radial < 1 + SWING_STABLE_G && s->tangent >= grip_y) ||
                s->radial - s->radial_low >= SWING_REVERSAL_DROP_G) {
                s->grip_loaded = false;
                s->radial_high = s->radial;
            }
        }
        s->radial_high = fmax(s->radial_high, s->radial);
        if (s->grip_loaded || s->tangent >= grip_y - SWING_BACK_TANGENT_G) {
            s->direction_since = 0;
            return;
        }
        if (!s->direction_since) s->direction_since = p->ms;
        if (p->ms - s->direction_since < SWING_DIRECTION_MS) return;
        s->phase = SWING_BACKSWING;
        s->direction_since = 0;
    }
    if (!s->descending) {
        s->radial_high = fmax(s->radial_high, s->radial);
        if (s->radial_high - s->radial < SWING_REVERSAL_DROP_G) return;
        s->descending = true;
        s->radial_low = s->radial;
    }
    s->radial_low = fmin(s->radial_low, s->radial);
    if (s->radial < s->radial_low + SWING_REVERSAL_RISE_G) {
        s->direction_since = 0;
        return;
    }
    if (!s->direction_since) s->direction_since = p->ms;
    if (p->ms - s->direction_since < SWING_DIRECTION_MS) return;
    s->phase = SWING_FORWARD;
    s->forward_ms = p->ms;
}

static void track_forward(Swing *s, const SwingSample *p, double mag, double e) {
    if (s->phase != SWING_FORWARD) return;
    if (p->clipped) {
        s->forward_clipped = true;
        if (!s->qualified) s->loaded = false;
        return;
    }
    unsigned first = s->count - 1, span;
    while (first > 0 && s->history[first-1].ms >= s->forward_ms &&
           p->ms - s->history[first].ms < SWING_WINDOW_MS &&
           s->history[first].ms - s->history[first-1].ms <= SWING_BRIDGE_MS)
        --first;
    s->mean_e = swing_window_mean(s->history + first, s->count - first, p->ms, &span);
    s->speed = span >= SWING_MIN_SPAN_MS ? swing_speed(s->mean_e) : 0;
    if (span < SWING_MIN_SPAN_MS || s->mean_e < SWING_FORWARD_G) {
        s->loaded = false;
        return;
    }
    if (!s->loaded) s->loaded_since = p->ms;
    s->loaded = true;
    s->last_loaded = p->ms;
    s->qualified |= p->ms - s->loaded_since >= SWING_FORWARD_MS;
    s->peak = fmax(s->peak, s->speed);

    /* Preserve the grip-relative excess-force/time-weighted aim proxy. */
    if (e >= SWING_FORWARD_G && s->count >= 2) {
        double lateral = p->g.x/mag - s->grip_g.x/fmax(norm(s->grip_g), 0.1);
        double dt = (double)(p->ms - s->history[s->count-2].ms);
        s->lateral_sum += lateral*e*dt;
        s->lateral_weight += e*dt;
    }
}
static void track_controls(Swing *s, const SwingSample *p, double mag, const SwingProfile *profile) {
    if (p->clipped) return;
    unsigned span;
    if (!s->roll_ref && hypot(p->g.x,p->g.z) >= SWING_ROLL_REF_G*mag &&
        swing_window_mean(s->history, s->count, p->ms, &span) >= SWING_ROLL_LOAD_G) {
        if (!s->roll_since) s->roll_since = p->ms;
        s->roll_x += p->g.x; s->roll_z += p->g.z;
        if (p->ms-s->roll_since >= SWING_ROLL_REF_MS) {
            s->locked_roll = atan2(s->roll_x, s->roll_z)*180/SWING_PI;
            s->roll_ref = true;
        }
    }
    double lateral=s->lateral_weight>0 ? s->lateral_sum/s->lateral_weight : 0;
    double yaw=swing_yaw(lateral);
    double hand=profile->left_handed ? -1 : 1;
    s->locked_aim=clamp(hand*yaw*SWING_AIM_MAX,-SWING_AIM_MAX,SWING_AIM_MAX);
    s->motion_roll=s->roll_ref ? delta_degrees(atan2(p->g.x,p->g.z)*180/SWING_PI,s->locked_roll) : 0;
    s->locked_spin=clamp(swing_roll_spin(s->motion_roll,profile)-hand*yaw*profile->spin_strength,-1,1);
}

static SwingResult finish(Swing *s, const SwingProfile *profile, int height, SwingRelease cause) {
    if (!s->qualified || s->last_ms - s->last_loaded > SWING_RELEASE_MS) {
        bool held = s->previous_b;
        swing_reset(s, held);
        return (SwingResult){0};
    }
    s->phase = SWING_PENDING;
    SwingResult r = {.thrown=true, .speed=s->peak, .power=throw_power(s, profile, height),
        .aim=s->locked_aim, .spin=s->locked_spin, .roll=s->motion_roll,
        .clipped=s->forward_clipped, .release=cause, .release_ms=s->last_ms};
    s->phase = SWING_SPENT;
    return r;
}

/* One report (real or bridged). An invalid accelerometer report still carries B. */
static SwingResult advance(Swing *s, const SwingSample *p, bool motion, const SwingProfile *profile, int height) {
    s->have_time = true;
    s->last_ms = p->ms;
    bool up = !p->b && s->previous_b;
    s->previous_b = p->b;
    if (!p->b) s->require_b_up = false;
    double mag = 0, e = 0;
    if (motion) {
        mag = norm(p->g);
        e = fmax(0, mag-1);
        push_point(s, p->ms, p->g, e, p->clipped);
        s->last_g = p->g;
        s->have_g = true;
        s->last_clipped = p->clipped;
    }
    if (s->phase == SWING_READY) {
        if (p->b && !s->require_b_up) grip(s);
        return (SwingResult){0};
    }
    if (motion && !p->clipped) track_direction(s, p);
    else s->direction_since = 0;
    if (motion) {
        track_forward(s, p, mag, e);
        track_controls(s, p, mag, profile);
    } else if (!s->qualified) s->loaded = false;
    if (up) return finish(s, profile, height, SWING_B_UP);
    if (!p->manual_release) {
        if (motion && !p->clipped && s->qualified && !s->loaded &&
            fabs(mag-1) < SWING_STABLE_G && s->speed <= SWING_AUTO_RATIO*s->peak &&
            delta_degrees(pitch(p->g), pitch(s->grip_g)) >= SWING_FINISH_DEG)
            return finish(s, profile, height, SWING_AUTO);
        if (p->ms - s->armed_ms >= SWING_TIMEOUT_MS)
            return finish(s, profile, height, SWING_TIMEOUT);
    }
    if (s->qualified && p->ms - s->last_loaded > SWING_RELEASE_MS) restart_motion(s);
    return (SwingResult){0};
}

SwingResult swing_feed(Swing *s, const SwingSample *in, const SwingProfile *profile, int height) {
    SwingResult r = {0};
    if (s->phase == SWING_SPENT) return r;
    SwingSample p = *in;
    double mag = norm(p.g);
    bool motion = p.accel_valid && isfinite(mag) && mag >= 0.1;
    if (s->have_time && p.ms <= s->last_ms) p.ms = s->last_ms+1; /* duplicate/backward stamp */
    if (s->have_time) {
        uint64_t gap = p.ms-s->last_ms;
        if (gap > SWING_RESET_GAP_MS) {
            /* Preserve the grip, but discard stale direction and forward power. */
            s->count = 0;
            if (swing_armed(s)) restart_motion(s);
            ++s->restarts;
        } else if (gap > SWING_GAP_MS && gap <= SWING_BRIDGE_MS && motion && s->have_g && s->count) {
            unsigned steps = (unsigned)((gap+SWING_BRIDGE_STEP_MS-1)/SWING_BRIDGE_STEP_MS);
            uint64_t t0 = s->last_ms;
            SwingVector g0 = s->last_g;
            bool clipped = s->last_clipped || p.clipped;
            for (unsigned k = 1; k < steps; ++k) {
                double u = (double)k/steps;
                SwingSample q = {.ms=t0+(uint64_t)llround(u*(double)gap),
                    .g={g0.x+(p.g.x-g0.x)*u, g0.y+(p.g.y-g0.y)*u, g0.z+(p.g.z-g0.z)*u},
                    .b=s->previous_b, .clipped=clipped, .accel_valid=true,
                    .manual_release=p.manual_release};
                r = advance(s, &q, true, profile, height);
                if (r.thrown) return r;
            }
            ++s->bridged;
        }
    }
    return advance(s, &p, motion, profile, height);
}

SwingCalibrationStatus swing_calibrate(SwingCalibration *cal, const SwingResult *toss,
                                      SwingCalibrationLimits lim, SwingProfile *profile) {
    if (!isfinite(lim.target_power) || lim.target_power <= 0 || lim.target_power > SWING_POWER_CAP ||
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
    profile->full_speed = clamp(SWING_POWER_GAIN * median/(lim.target_power * swing_type_scale(1)), lim.full_speed_floor, lim.clip_ceiling);
    return SWING_CAL_DONE;
}
