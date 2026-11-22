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
    return isfinite(peak) && isfinite(full) && full > 0 ? clamp(SWING_POWER_GAIN * (peak/full), 0, 1) : 0;
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
    s->phase = SWING_READY;
    s->require_b_up = up;
}
bool swing_armed(const Swing *s) { return s->phase == SWING_GRIP || s->phase == SWING_PENDING; }

static bool stable_for(const Swing *s, SwingVector *mean, unsigned duration) {
    if (!s->count) return false;
    uint64_t now = s->history[s->count-1].ms;
    unsigned first = 0;
    while (first+1 < s->count && now-s->history[first+1].ms >= duration) ++first;
    if (now-s->history[first].ms < duration) return false;
    *mean = (SwingVector){0,0,0};
    for (unsigned i = first; i < s->count; ++i) {
        if (s->history[i].clipped || fabs(norm(s->history[i].g)-1) >= SWING_STABLE_G) return false;
        for (unsigned j = first; j < i; ++j)
            if (angle(s->history[i].g, s->history[j].g) >= SWING_STABLE_DEG) return false;
        mean->x += s->history[i].g.x;
        mean->y += s->history[i].g.y;
        mean->z += s->history[i].g.z;
    }
    return norm(*mean) > 0;
}
bool swing_preview(const Swing *s, const SwingProfile *profile, double *power, double *aim) {
    if (!swing_armed(s) || !s->confirmed || !(s->in_lobe || s->lobe_valid)) return false;
    double peak = fmax(s->peak, s->lobe_peak);
    if (!(peak > 0)) return false;
    *power = s->lobe_clipped ? 1 : swing_power(peak, profile->full_speed);
    *aim = s->locked_aim;
    return true;
}
/* A Bowling-style reversal need not pause. Track a behind-grip attitude near
   1 g, then a sequence of forward pitch changes. A single jump (including
   one caused by hard backswing braking) cannot establish the return. */
static void clear_return(Swing *s, double pitch) {
    s->return_started_ms=s->return_ms=0;
    s->return_travel=s->return_peak=s->return_lateral_sum=0;
    s->return_high_pitch=pitch;
}
static void track_return(Swing *s, const SwingSample *p, double mag, double e) {
    double relative=delta_degrees(pitch(p->g),pitch(s->grip_g));
    double step=delta_degrees(relative,s->previous_pitch);
    s->previous_pitch=relative;
    if (relative<=-SWING_FINISH_DEG && fabs(mag-1)<SWING_RETURN_BACK_G &&
        (!s->back_motion_ms || relative<s->back_pitch-SWING_RETURN_STEP_DEG)) {
        s->back_motion_ms=p->ms;
        s->back_pitch=relative;
        clear_return(s,relative);
    }
    if (relative<s->return_high_pitch-SWING_RETURN_BACKTRACK_DEG ||
        fabs(step)>SWING_RETURN_MAX_STEP_DEG) {
        clear_return(s,relative);
    } else if (step>=SWING_RETURN_STEP_DEG) {
        if (!s->return_started_ms) s->return_started_ms=p->ms;
        s->return_travel+=step;
        s->return_ms=p->ms;
        s->return_high_pitch=fmax(s->return_high_pitch,relative);
        s->return_peak=fmax(s->return_peak,e);
        s->return_lateral_sum+=step*(p->g.x/mag-s->grip_g.x/norm(s->grip_g));
    }
}
static bool forward_return(const Swing *s) {
    bool advanced=s->back_motion_ms ? s->previous_pitch>=s->back_pitch+SWING_RETURN_DEG :
        s->previous_pitch>=SWING_FINISH_DEG && s->return_peak>=SWING_NOISE_G;
    return advanced && s->return_ms && s->return_ms-s->return_started_ms>=SWING_RETURN_MS &&
        s->last_ms-s->return_ms<=SWING_RELEASE_MS && s->return_travel>=SWING_RETURN_DEG;
}
bool swing_forward_motion(const Swing *s) { return s->confirmed || forward_return(s); }
static double swing_yaw(double lateral) {
    return fabs(lateral)<SWING_LATERAL_DEAD ? 0 : clamp(lateral/SWING_LATERAL_FULL,-1,1);
}
const char *swing_phase_name(const Swing *s) {
    switch (s->phase) {
    case SWING_READY: return "READY";
    case SWING_PENDING: return "RELEASE";
    case SWING_SPENT: return "SPENT";
    case SWING_GRIP: break;
    }
    if ((s->confirmed && (s->in_lobe || s->lobe_valid)) || forward_return(s)) return "FORWARD";
    if (s->lobes || s->back_apex || s->back_motion_ms) return "BACKSWING";
    return "GRIP";
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
/* Whatever the remote is doing when B goes down: mean gravity of the last 100 ms of reports
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
static void restart_motion(Swing *s) {
    s->count = 0;
    s->in_lobe = s->dipping = false;
    s->mean_e = s->speed = 0;
    s->back_motion_ms = 0;
    clear_return(s, s->previous_pitch);
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
    s->previous_pitch = 0;
    clear_return(s, 0);
    s->phase = SWING_GRIP;
}

/* ---------- throws ---------- */
/* Weak throw: B came up without a classified forward stroke. Power from whatever speed was
   measured (a B tap gives the minimum), capped well below a real swing. */
static SwingResult finish_weak(Swing *s, const SwingProfile *profile, double speed) {
    s->phase = SWING_SPENT;
    return (SwingResult){.thrown=true, .fallback=true, .speed=speed,
        .power=clamp(SWING_FALLBACK_POWER_SCALE*swing_power(speed, profile->full_speed),
                     SWING_FALLBACK_MIN_POWER, SWING_FALLBACK_MAX_POWER),
        .aim=s->locked_aim, .spin=s->locked_spin, .roll=s->motion_roll,
        .release=SWING_B_UP, .release_ms=s->release_ms};
}
static SwingResult finish_fallback(Swing *s, const SwingProfile *profile) {
    if (!(s->return_travel > 0)) return finish_weak(s, profile, swing_speed(s->return_peak));
    double lateral=s->return_lateral_sum/s->return_travel;
    double yaw=swing_yaw(lateral);
    double hand=profile->left_handed ? -1 : 1;
    SwingResult r=finish_weak(s, profile, swing_speed(s->return_peak));
    r.aim=clamp(hand*yaw*SWING_AIM_MAX,-SWING_AIM_MAX,SWING_AIM_MAX);
    r.spin=clamp(swing_roll_spin(s->motion_roll,profile)-hand*yaw*profile->spin_strength,-1,1);
    return r;
}
static SwingResult finish(Swing *s, const SwingProfile *profile, SwingRelease cause) {
    uint64_t reference_ms=cause==SWING_B_UP ? s->release_ms : s->last_ms;
    bool recent = s->lobe_valid && s->peak > 0 && reference_ms>=s->lobe_end &&
        reference_ms-s->lobe_end <= SWING_RELEASE_MS;
    if (recent && s->confirmed) {
        s->phase = SWING_SPENT;
        return (SwingResult){.thrown=true, .speed=s->peak,
            .power=s->lobe_clipped ? 1 : swing_power(s->peak, profile->full_speed),
            .aim=s->locked_aim, .spin=s->locked_spin, .roll=s->motion_roll,
            .clipped=s->lobe_clipped, .release=cause, .release_ms=reference_ms};
    }
    SwingResult r = finish_weak(s, profile, recent ? s->peak : swing_speed(s->return_peak));
    r.release = cause;
    r.release_ms = reference_ms;
    return r;
}

/* ---------- strokes ---------- */
static void start_lobe(Swing *s, uint64_t ms) {
    s->in_lobe = true;
    s->dipping = false;
    s->lobe_start = ms;
    s->lobe_peak = s->lobe_peak_e = 0;
    s->lateral_sum = s->lateral_weight = 0;
    s->lobe_valid = s->lobe_clipped = false;
    s->peak = 0;
    ++s->lobes;
    /* After a back apex (a calm pause behind, or the speed dip of a continuous swing) the
       stroke is forward. A lone first stroke still needs follow-through confirmation. */
    s->confirmed = s->back_apex || s->lobes >= 2;
}
static void track_strokes(Swing *s, const SwingSample *p, double mag, double e, unsigned span) {
    if (s->phase == SWING_PENDING || p->clipped || span < SWING_MIN_SPAN_MS) {
        if (p->clipped && s->lobe_valid && s->in_lobe) s->lobe_clipped = true;
        return;
    }
    bool active = s->mean_e >= SWING_FORWARD_G;
    /* A continuous Bowling swing never drops below SWING_FORWARD_G at the back apex: the
       backswing and forward stroke merge. Split them where the load dips to half its peak. */
    if (s->in_lobe && s->lobe_valid && s->mean_e < SWING_DIP_RATIO*s->lobe_peak_e) {
        s->in_lobe = false;
        s->dipping = true;
        s->dip_e = s->mean_e;
    }
    if (s->dipping) {
        s->dip_e = fmin(s->dip_e, s->mean_e);
        if (!active) s->dipping = false;
        else if (s->mean_e >= SWING_RISE_RATIO*s->dip_e && s->mean_e >= s->dip_e+SWING_NOISE_G)
            start_lobe(s, p->ms);
    }
    if (active && !s->in_lobe && !s->dipping) start_lobe(s, p->ms);
    if (!active) { s->in_lobe = false; return; }
    if (!s->in_lobe) return;
    /* Grip-relative lateral acceleration is a candidate swing-yaw proxy.
       Weight by excess force, never use this signal for power. */
    if (e >= SWING_FORWARD_G && s->count >= 2) {
        double lateral=p->g.x/mag-s->grip_g.x/norm(s->grip_g);
        double dt=(double)(p->ms-s->history[s->count-2].ms);
        s->lateral_sum+=lateral*e*dt;
        s->lateral_weight+=e*dt;
    }
    s->lobe_peak = fmax(s->lobe_peak, s->speed);
    s->lobe_peak_e = fmax(s->lobe_peak_e, s->mean_e);
    s->lobe_end = p->ms;
    if (p->ms-s->lobe_start >= SWING_LOBE_MS) {
        s->lobe_valid = true;
        s->peak = s->lobe_peak;
    }
}
static void track_controls(Swing *s, const SwingSample *p, double mag, const SwingProfile *profile) {
    if (s->phase == SWING_PENDING || p->clipped) return;
    if (!s->roll_ref && s->mean_e >= SWING_FORWARD_G &&
        sqrt(p->g.x*p->g.x+p->g.z*p->g.z) >= SWING_ROLL_REF_G*mag) {
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

/* One report (real or bridged). motion=false: accel unusable, B still counts. */
static SwingResult advance(Swing *s, const SwingSample *p, bool motion, const SwingProfile *profile) {
    SwingResult r = {0};
    s->have_time = true;
    s->last_ms = p->ms;
    bool up = !p->b && s->previous_b;
    s->previous_b = p->b;
    if (!p->b) s->require_b_up = false;
    double mag = 0, e = 0;
    unsigned span = 0;
    if (motion) {
        mag = norm(p->g);
        e = fmax(0, mag-1);
        push_point(s, p->ms, p->g, e, p->clipped);
        s->last_g = p->g;
        s->have_g = true;
        s->last_clipped = p->clipped;
        s->mean_e = swing_window_mean(s->history, s->count, p->ms, &span);
        s->speed = span >= SWING_MIN_SPAN_MS && s->mean_e >= SWING_NOISE_G ? swing_speed(s->mean_e) : 0;
    }
    if (s->phase == SWING_READY) {
        if (p->b && !s->require_b_up) grip(s); /* this report: 0 ms from B down to grip */
        return r;
    }
    bool forward_finish = false;
    if (motion) {
        if (s->phase == SWING_GRIP && !p->clipped) track_return(s, p, mag, e);
        track_strokes(s, p, mag, e, span);
        track_controls(s, p, mag, profile);
        /* Optional cues from a calm attitude (a paused backswing, a held follow-through). */
        SwingVector calm;
        if (s->mean_e < SWING_NOISE_G && stable_for(s, &calm, SWING_APEX_MS)) {
            double dp = delta_degrees(pitch(calm), pitch(s->grip_g));
            if (dp <= -SWING_FINISH_DEG) {
                s->back_apex = true;
                s->in_lobe = s->dipping = false;
                s->confirmed = s->lobe_valid = s->lobe_clipped = false;
                s->peak = 0;
                s->lateral_sum = s->lateral_weight = 0;
            } else if (s->lobe_valid && dp >= SWING_FINISH_DEG) {
                s->confirmed = true;
                forward_finish = true;
            }
        }
    }
    if (up && s->phase == SWING_GRIP) {
        s->release_ms = p->ms;
        if (forward_return(s)) {
            bool recent_lobe=s->lobe_valid && s->lobe_start>s->back_motion_ms &&
                p->ms-s->lobe_end<=SWING_RELEASE_MS;
            /* A lone lobe still uses normal follow-through confirmation. */
            if (recent_lobe && !s->back_motion_ms && !s->confirmed) recent_lobe=false;
            if (!recent_lobe) return finish_fallback(s,profile);
            s->confirmed=true;
        }
        /* Releasing B always throws something: no stroke at all is the weak lob. */
        if (!s->lobe_valid) return finish(s, profile, SWING_B_UP);
        s->phase = SWING_PENDING;
    }
    if (s->phase == SWING_PENDING) {
        if (s->confirmed || p->ms-s->release_ms >= SWING_RELEASE_MS) return finish(s, profile, SWING_B_UP);
    } else if (!p->manual_release && s->lobe_valid && s->confirmed && forward_finish && p->ms-s->lobe_start >= SWING_WINDOW_MS &&
               s->speed <= SWING_AUTO_RATIO*s->peak) return finish(s, profile, SWING_AUTO);
    /* Bowling input has no timeout: holding B just keeps the grip. */
    if (s->phase==SWING_GRIP && !p->manual_release && p->ms-s->armed_ms >= SWING_TIMEOUT_MS)
        return finish(s,profile,SWING_TIMEOUT);
    return r;
}

SwingResult swing_feed(Swing *s, const SwingSample *in, const SwingProfile *profile) {
    SwingResult r = {0};
    if (s->phase == SWING_SPENT) return r;
    SwingSample p = *in;
    double mag = norm(p.g);
    bool motion = p.accel_valid && isfinite(mag) && mag >= 0.1;
    if (s->have_time && p.ms <= s->last_ms) p.ms = s->last_ms+1; /* duplicate/backward stamp */
    if (s->have_time) {
        uint64_t gap = p.ms-s->last_ms;
        if (gap > SWING_RESET_GAP_MS) {
            /* A long stall: the motion so far no longer describes this swing. Keep the grip;
               if B is up now, the release below still throws. */
            restart_motion(s);
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
                r = advance(s, &q, true, profile);
                if (r.thrown) return r;
            }
            ++s->bridged;
        }
    }
    return advance(s, &p, motion, profile);
}

SwingCalibrationStatus swing_calibrate(SwingCalibration *cal, const SwingResult *toss,
                                      SwingCalibrationLimits lim, SwingProfile *profile) {
    if (!isfinite(lim.target_power) || lim.target_power <= 0 || lim.target_power > 1 ||
        !isfinite(lim.full_speed_floor) || lim.full_speed_floor <= 0 ||
        !isfinite(lim.clip_ceiling) || lim.clip_ceiling < lim.full_speed_floor)
        return SWING_CAL_BAD_LIMITS;
    if (!toss->thrown || toss->fallback || toss->clipped || !isfinite(toss->speed) || toss->speed <= 0) {
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
    profile->full_speed = clamp(SWING_POWER_GAIN * median/lim.target_power, lim.full_speed_floor, lim.clip_ceiling);
    return SWING_CAL_DONE;
}
