#include "swing.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

static double clamp(double x, double low, double high) { return fmax(low, fmin(high, x)); }
static double norm(SwingVector v) { return sqrt(v.x*v.x + v.y*v.y + v.z*v.z); }
static double degrees(double radians) { return radians*180/SWING_PI; }
static double tip_pitch(SwingVector g) { return degrees(atan2(-g.y, hypot(g.x, g.z))); }

SwingProfile swing_default_profile(void) {
    return (SwingProfile){SWING_DEFAULT_FULL_SPEED, 1, false};
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
    return isfinite(peak) && isfinite(full) && full > 0 ?
        clamp(SWING_POWER_GAIN * (peak/full), 0, SWING_POWER_CAP) : 0;
}
double swing_roll_spin(double roll, const SwingProfile *profile) {
    if (!isfinite(roll) || !isfinite(profile->spin_strength)) return 0;
    double strength = clamp((fabs(roll)-SWING_ROLL_DEAD_DEG)/
                            (SWING_ROLL_FULL_DEG-SWING_ROLL_DEAD_DEG), 0, 1);
    double hand = profile->left_handed ? -1 : 1;
    return -hand*copysign(strength, roll)*clamp(profile->spin_strength, 0, 1);
}

const char *swing_phase_name(const Swing *s) {
    static const char *const names[] = {
        "READY", "GRIP", "BACKSWING", "TURN", "FORWARD", "PAST BOTTOM", "RESOLVE", "DONE"
    };
    return names[s->phase];
}
const char *swing_reject_name(SwingReject reason) {
    static const char *const names[SWING_REJECT_COUNT] = {
        "-", "NO SWING", "LET GO ON BACKSWING", "LET GO BEFORE BOTTOM",
        "FORWARD TOO WEAK", "LET GO TOO LATE", "REPORT GAP"
    };
    return names[reason];
}

static void set_phase(Swing *s, SwingPhase phase) {
    s->phase = phase;
    s->phase_ms = s->last_ms;
}

void swing_reset(Swing *s, bool require_b_up) {
    memset(s, 0, sizeof(*s));
    s->require_b_up = require_b_up;
}

void swing_cancel(Swing *s, bool require_b_up) {
    set_phase(s, SWING_READY);
    s->have_time = s->have_g = s->pitch_static = false;
    s->still_ms = 0;
    s->previous_b = false;
    s->require_b_up = require_b_up;
}

bool swing_armed(const Swing *s) {
    return s->phase != SWING_READY && s->phase != SWING_SPENT;
}

static SwingResult reject(Swing *s, SwingReject reason) {
    s->last_reject = reason;
    ++s->rejects[reason];
    set_phase(s, SWING_READY);
    s->require_b_up = s->previous_b;
    return (SwingResult){.reject=reason};
}

void swing_report_gap(Swing *s) {
    if (swing_armed(s)) {
        ++s->restarts;
        reject(s, SWING_REJECT_REPORT_GAP);
    }
    if (s->phase != SWING_SPENT) swing_cancel(s, true);
}

void swing_expire(Swing *s, uint64_t now) {
    if (s->phase == SWING_RESOLVE && now >= s->release_ms &&
        now-s->release_ms >= SWING_RESOLVE_MS) reject(s, SWING_REJECT_BEFORE_BOTTOM);
}

static double lateral_mean(const Swing *s) {
    return s->lateral_weight > 0 ? s->lateral_sum/s->lateral_weight : 0;
}

static double throw_aim(const Swing *s) {
    /* Raw +X points left for either hand, while positive game yaw goes right. */
    double lateral = lateral_mean(s) - s->grip_g.x/fmax(norm(s->grip_g), 0.1);
    return fabs(lateral) < SWING_LATERAL_DEAD ? 0 :
        -SWING_AIM_MAX * clamp(lateral/SWING_LATERAL_FULL, -1, 1);
}

static double throw_speed(const Swing *s) {
    return fmax(s->peak, s->fitted_speed);
}

static double throw_power(const Swing *s, const SwingProfile *profile, int height) {
    return swing_power(throw_speed(s), profile->full_speed*swing_type_scale(height));
}

bool swing_preview(const Swing *s, const SwingProfile *profile, int height, double *power, double *aim) {
    if (s->phase != SWING_FORWARD &&
        (s->phase != SWING_PAST_BOTTOM || s->last_ms-s->phase_ms > SWING_RELEASE_WINDOW_MS))
        return false;
    if (s->peak < swing_speed(SWING_QUALIFY_E_G)) return false;
    *power = throw_power(s, profile, height);
    *aim = throw_aim(s);
    return true;
}

static SwingResult finish(Swing *s, const SwingProfile *profile, int height) {
    if (s->peak < swing_speed(SWING_QUALIFY_E_G)) return reject(s, SWING_REJECT_TOO_WEAK);
    double roll = s->release_roll-degrees(atan(lateral_mean(s)));
    set_phase(s, SWING_SPENT);
    return (SwingResult){.thrown=true, .power=throw_power(s, profile, height),
        .speed=throw_speed(s), .aim=throw_aim(s), .spin=swing_roll_spin(roll, profile),
        .roll=roll, .clipped=s->forward_clipped, .release=SWING_B_UP, .release_ms=s->release_ms};
}

static void observe_pitch(Swing *s, SwingVector g, double magnitude, uint64_t dt) {
    if (fabs(magnitude-1) < SWING_STATIC_G) {
        double alpha = s->pitch_static ? 0.3 : 1;
        s->static_g.x += alpha*(g.x-s->static_g.x);
        s->static_g.y += alpha*(g.y-s->static_g.y);
        s->static_g.z += alpha*(g.z-s->static_g.z);
        if (s->still_ms < SWING_STATIC_MS) s->still_ms += dt;
        s->pitch_static = s->still_ms >= SWING_STATIC_MS;
        g = s->static_g;
    } else {
        s->still_ms = 0;
        s->pitch_static = false;
    }
    s->pitch = tip_pitch(g);
    s->face_pitch = degrees(atan2(-g.y, g.z));
    s->roll = degrees(atan2(g.x, g.z));
}

static void grip(Swing *s) {
    /* Keep report timing, the filtered gravity estimate, and reject totals. */
    memset(&s->release_ms, 0, sizeof(*s)-offsetof(Swing, release_ms));
    s->last_reject = SWING_REJECT_NONE;
    s->grip_g = s->pitch_static ? s->static_g : s->have_g ? s->last_g : (SwingVector){0,-1,0};
    s->grip_pitch = tip_pitch(s->grip_g);
    s->tangent_sign = 1; /* provisional until grip or turnaround establishes the sign */
    set_phase(s, SWING_GRIP);
}

static double project_radial(const Swing *s, SwingVector g) {
    return g.y*s->ny + g.z*s->nz;
}

static double project_tangent(const Swing *s, SwingVector g) {
    return s->tangent_sign*(-g.y*s->nz + g.z*s->ny);
}

static void decompose(Swing *s, SwingVector g, double plane_load) {
    s->radial = s->have_axis ? project_radial(s, g) : plane_load;
    s->tangent = s->have_axis ? project_tangent(s, g) : 0;
    s->excess = s->have_axis ? fmax(0, s->radial-1) : 0;
}

static void observe_arm(Swing *s, uint64_t dt) {
    if (!s->observer_valid) return;
    double angle = s->arm_angle*SWING_PI/180;
    s->speed = swing_speed(fmax(0, s->radial-cos(angle)));
    bool forward = s->phase == SWING_FORWARD || s->phase == SWING_RESOLVE ||
                   s->phase == SWING_PAST_BOTTOM;
    double travelled = degrees(s->speed/SWING_RADIUS_M*(double)dt/1000);
    s->arm_angle = clamp(s->arm_angle + (forward ? travelled : -travelled), -90, 90);
}

static double crest_drop(double peak) {
    return fmax(SWING_CREST_DROP_G, SWING_CREST_DROP_FRACTION*(peak-1));
}

static bool near_rail(const SwingSample *p) {
    return p->clipped || fabs(p->g.y) >= SWING_RAIL_G-SWING_RAIL_NEAR_G ||
                         fabs(p->g.z) >= SWING_RAIL_G-SWING_RAIL_NEAR_G;
}

static SwingPoint *point(Swing *s, unsigned offset) {
    return &s->history[(s->first+offset)%SWING_HISTORY];
}

static const SwingPoint *read_point(const Swing *s, unsigned offset) {
    return &s->history[(s->first+offset)%SWING_HISTORY];
}

static void add_point(Swing *s, const SwingSample *p) {
    if (s->count == SWING_HISTORY) {
        s->first = (s->first+1)%SWING_HISTORY;
        --s->count;
    }
    *point(s, s->count) = (SwingPoint){p->ms, p->g, s->radial, near_rail(p)};
    ++s->count;
}

static void measure_forward(Swing *s, const SwingPoint *p, uint64_t dt, unsigned count) {
    double sum = 0;
    unsigned n = 0;
    while (count > 0) {
        const SwingPoint *q = read_point(s, --count);
        if (p->ms-q->ms > SWING_POWER_WINDOW_MS) break;
        sum += fmax(0, q->radial-1);
        ++n;
    }
    if (n >= 2) s->peak = fmax(s->peak, swing_speed(sum/n));
    double excess = fmax(0, p->radial-1);
    if (excess > 0.1) {
        double weight = excess*(double)dt;
        s->lateral_sum += p->g.x/fmax(p->radial, 0.5)*weight;
        s->lateral_weight += weight;
    }
    /* A railed X reading cannot give a reliable twist angle. */
    if (fabs(p->g.x) < SWING_RAIL_G-SWING_RAIL_NEAR_G)
        s->release_roll = degrees(atan2(p->g.x, fmax(p->radial, 0.3)));
}

static double fitted_peak(const Swing *s, double half, double rail) {
    /* Fit radial load = a + b*t + c*t^2 to the unclipped shoulders.
       Scale t by 100 ms and subtract the bottom time to condition the fit. */
    double moments[5] = {0}, weighted[3] = {0};
    unsigned count = 0, before = 0, after = 0;
    for (unsigned i = 0; i < s->count; ++i) {
        const SwingPoint *p = read_point(s, i);
        if (p->clipped || p->radial <= half || p->radial >= rail-SWING_FIT_MARGIN_G) continue;
        double t = ((double)p->ms-(double)s->bottom_ms)/100, power = 1;
        for (unsigned j = 0; j < 5; ++j) {
            moments[j] += power;
            if (j < 3) weighted[j] += power*p->radial;
            power *= t;
        }
        ++count;
        if (t < 0) ++before;
        if (t > 0) ++after;
    }
    if (count < 4 || !before || !after) return 0;
    double m[3][4] = {
        {moments[0], moments[1], moments[2], weighted[0]},
        {moments[1], moments[2], moments[3], weighted[1]},
        {moments[2], moments[3], moments[4], weighted[2]}
    };
    for (unsigned col = 0; col < 3; ++col) {
        if (fabs(m[col][col]) < 1e-9) return 0;
        for (unsigned row = col+1; row < 3; ++row) {
            double factor = m[row][col]/m[col][col];
            for (unsigned j = col; j < 4; ++j) m[row][j] -= factor*m[col][j];
        }
    }
    double c = m[2][3]/m[2][2];
    if (c >= 0) return 0;
    double b = (m[1][3]-m[1][2]*c)/m[1][1];
    double a = (m[0][3]-m[0][1]*b-m[0][2]*c)/m[0][0];
    double peak = a-b*b/(4*c);
    if (!isfinite(peak) || peak < s->forward_peak) return 0;
    return swing_speed(fmin(peak, 2*SWING_RAIL_G)-1);
}

static void confirm_bottom(Swing *s) {
    const uint64_t start = read_point(s, 0)->ms;
    double half = 1 + 0.5*(s->forward_peak-1), weight = 0, total = 0;
    double rail = 0;
    for (unsigned i = 0; i < s->count; ++i) {
        const SwingPoint *p = read_point(s, i);
        if (p->radial <= half) continue;
        double w = p->radial-half;
        weight += w;
        total += w*(double)(p->ms-start);
        if (p->clipped) rail = fmax(rail, p->radial);
    }
    s->bottom_ms = start+(uint64_t)(weight > 0 ? total/weight : 0);
    s->have_bottom = true;
    ++s->passes;
    s->forward_clipped = rail > 0;
    if (s->forward_clipped) s->fitted_speed = fitted_peak(s, half, fmin(rail, SWING_RAIL_G));
    if (s->phase != SWING_RESOLVE) set_phase(s, SWING_PAST_BOTTOM);
    s->turn_min = INFINITY;
    s->turn_rise = 0;
}

static void track_first_lobe(Swing *s, const SwingSample *p, double load) {
    if (!s->in_lobe && load < SWING_LOBE_ON_G) return;
    if (!s->in_lobe) {
        s->in_lobe = true;
        s->lobe_start_ms = p->ms;
        s->lobe_peak = 0;
        s->top_y = s->top_z = 0;
        s->count = s->first = 0;
        set_phase(s, SWING_BACKSWING);
    }
    add_point(s, p);
    if (load > s->lobe_peak) {
        s->lobe_peak = load;
        s->lobe_peak_ms = p->ms;
        s->lobe_ny = p->g.y/load;
        s->lobe_nz = p->g.z/load;
    }
    double weight = load-(1+0.5*(s->lobe_peak-1));
    if (weight > 0) {
        s->top_y += weight*p->g.y/load;
        s->top_z += weight*p->g.z/load;
    }
    if (load > s->lobe_peak-crest_drop(s->lobe_peak)) return;

    s->in_lobe = false;
    if (s->lobe_peak < SWING_LOBE_MIN_PEAK_G || p->ms-s->lobe_start_ms < SWING_LOBE_MIN_MS)
        return;

    double top = hypot(s->top_y, s->top_z);
    s->ny = top > 0 ? s->top_y/top : s->lobe_ny;
    s->nz = top > 0 ? s->top_z/top : s->lobe_nz;
    double grip_load = hypot(s->grip_g.y, s->grip_g.z);
    double uy = grip_load > 0.3 ? s->grip_g.y/grip_load : -1;
    double uz = grip_load > 0.3 ? s->grip_g.z/grip_load : 0;
    s->grip_turn = degrees(acos(clamp(uy*s->ny+uz*s->nz, -1, 1)));
    bool back = s->grip_turn >= SWING_BACK_ROTATION_DEG;
    if (!back && (s->grip_pitch >= 45 || s->lobe_peak < SWING_AMBIGUOUS_PEAK_G)) return;

    s->have_axis = true;
    double tangent = -uy*s->nz+uz*s->ny;
    s->tangent_sign = tangent < 0 ? -1 : 1;
    if (fabs(tangent) >= SWING_TANGENT_SIGN_G) s->sign_source = SWING_SIGN_GRIP;
    decompose(s, p->g, load);
    s->observer_valid = true;
    s->speed = swing_speed(s->lobe_peak-1);
    s->arm_angle = (back ? -1 : 1)*degrees(s->speed/SWING_RADIUS_M*
                                                   (double)(p->ms-s->lobe_peak_ms)/1000);
    if (back) {
        double minimum = 0, maximum = 0, half = 1+0.5*(s->lobe_peak-1);
        for (unsigned i = 0; i < s->count; ++i) {
            const SwingPoint *q = read_point(s, i);
            if (q->radial <= half) continue;
            double angle = degrees(atan2(-q->g.y*s->nz+q->g.z*s->ny, project_radial(s, q->g)));
            minimum = fmin(minimum, angle);
            maximum = fmax(maximum, angle);
        }
        /* A large wrist turn can leave the learned direction early-biased.
           Use a stricter release gate if the lobe top shows that rotation. */
        s->unstable_axis = s->grip_turn > SWING_LARGE_GRIP_ROTATION_DEG &&
                           maximum-minimum > SWING_BACK_AXIS_SPREAD_DEG;
        s->passes = 1;
        s->turn_min = INFINITY;
        s->count = s->first = 0;
        set_phase(s, SWING_TURN);
        return;
    }

    /* A tip-forward grip near the bottom can start with a forward pass.
       Its samples only become eligible for power once the crest confirms it. */
    for (unsigned i = 0; i < s->count; ++i) {
        SwingPoint *q = point(s, i);
        q->radial = project_radial(s, q->g);
        s->forward_peak = fmax(s->forward_peak, q->radial);
        uint64_t dt = i > 0 ? q->ms-read_point(s, i-1)->ms : 0;
        measure_forward(s, q, dt, i+1);
    }
    confirm_bottom(s);
}

static bool turn_confirmed(Swing *s, double load, double direction) {
    if (load < s->turn_min) {
        s->turn_min = load;
        s->turn_radial = s->radial;
        s->turn_rise = 0;
        s->turn_tangent = 0;
        return false;
    }

    s->turn_tangent += s->tangent;
    bool opposing = s->sign_source != SWING_SIGN_UNKNOWN && direction*s->tangent < -SWING_BACK_PULL_G;
    if (load >= s->turn_min+SWING_TURN_RISE_G && !opposing) ++s->turn_rise;
    else s->turn_rise = 0;
    return s->turn_rise >= SWING_TURN_REPORTS;
}

static void track_motion(Swing *s, const SwingSample *p, double magnitude, uint64_t dt) {
    double load = hypot(p->g.y, p->g.z);
    decompose(s, p->g, load);
    observe_arm(s, dt);

    if (s->phase == SWING_GRIP) {
        if (fabs(magnitude-1) > SWING_ONSET_G) {
            if (!s->onset) s->onset_ms = p->ms;
            s->onset = true;
            if (p->ms-s->onset_ms >= SWING_ONSET_MS) set_phase(s, SWING_BACKSWING);
        } else s->onset = false;
    }
    if (!s->have_axis && (s->phase == SWING_GRIP || s->phase == SWING_BACKSWING))
        track_first_lobe(s, p, load);

    /* A dip while the remote is still heavily loaded can be a clipped back
       pass, so wait for a lower radial minimum before opening forward power. */
    if (s->phase == SWING_TURN && turn_confirmed(s, load, 1) &&
        s->turn_radial < SWING_TURN_MAX_RADIAL_G) {
        if (s->sign_source == SWING_SIGN_UNKNOWN) {
            if (s->turn_tangent < 0) s->tangent_sign = -s->tangent_sign;
            s->sign_source = SWING_SIGN_TURN;
            s->tangent = project_tangent(s, p->g);
        }
        s->arm_angle = -degrees(acos(clamp(s->turn_radial, -1, 1)));
        s->speed = 0;
        set_phase(s, SWING_FORWARD);
    }

    if (s->phase == SWING_FORWARD || s->phase == SWING_RESOLVE) {
        add_point(s, p);
        s->forward_peak = fmax(s->forward_peak, s->radial);
        if (s->phase == SWING_FORWARD && p->b)
            measure_forward(s, read_point(s, s->count-1), dt, s->count);
        if (s->forward_peak >= SWING_LOBE_MIN_PEAK_G &&
            s->radial <= s->forward_peak-crest_drop(s->forward_peak))
            confirm_bottom(s);
        return;
    }
    if (s->phase == SWING_PAST_BOTTOM && p->ms != s->phase_ms) {
        /* The tangential braking spike can raise total load even as the hand
           comes to rest. The return starts when radial load rises again. */
        bool turned = turn_confirmed(s, s->radial, -1);
        if (turned && s->turn_min < SWING_LOBE_ON_G) {
            s->window_closed = true;
            set_phase(s, SWING_BACKSWING);
        } else if (p->b && p->ms-s->phase_ms <= SWING_RELEASE_WINDOW_MS) {
            add_point(s, p);
            measure_forward(s, read_point(s, s->count-1), dt, s->count);
        }
    }
}

static SwingResult resolve(Swing *s, const SwingProfile *profile, int height) {
    if (s->last_ms-s->release_ms > SWING_RESOLVE_MS)
        return reject(s, SWING_REJECT_BEFORE_BOTTOM);
    if (s->have_bottom) {
        unsigned tolerance = s->unstable_axis ? 0 : SWING_BOTTOM_TOL_MS;
        if (s->bottom_ms > s->release_ms && s->bottom_ms-s->release_ms > tolerance)
            return reject(s, SWING_REJECT_BEFORE_BOTTOM);
        return finish(s, profile, height);
    }
    if (s->last_ms-s->release_ms == SWING_RESOLVE_MS)
        return reject(s, SWING_REJECT_BEFORE_BOTTOM);
    return (SwingResult){0};
}

/* An invalid accelerometer report still carries the B edge. */
static SwingResult advance(Swing *s, const SwingSample *p, bool motion, double magnitude,
                           const SwingProfile *profile, int height) {
    uint64_t dt = s->have_time ? p->ms-s->last_ms : 0;
    s->have_time = true;
    s->last_ms = p->ms;
    bool up = s->previous_b && !p->b;
    bool down = !s->previous_b && p->b;
    s->previous_b = p->b;
    if (!p->b) s->require_b_up = false;
    s->have_g = motion;
    if (motion) {
        s->last_g = p->g;
        s->last_clipped = p->clipped;
        observe_pitch(s, p->g, magnitude, dt);
    } else {
        s->still_ms = 0;
        s->pitch_static = false;
        s->onset = false;
        s->turn_rise = 0;
    }
    if (s->phase == SWING_READY) {
        if (down && !s->require_b_up) grip(s);
        if (motion) decompose(s, p->g, hypot(p->g.y, p->g.z));
        return (SwingResult){0};
    }

    if (s->phase == SWING_RESOLVE && down) return reject(s, SWING_REJECT_BEFORE_BOTTOM);
    if (motion) track_motion(s, p, magnitude, dt);
    if (s->phase == SWING_RESOLVE) return resolve(s, profile, height);
    if (!up) return (SwingResult){0};

    s->release_ms = p->ms;
    if (s->window_closed) return reject(s, SWING_REJECT_TOO_LATE);
    switch (s->phase) {
    case SWING_GRIP: return reject(s, SWING_REJECT_NO_SWING);
    case SWING_BACKSWING:
    case SWING_TURN: return reject(s, SWING_REJECT_BACKSWING);
    case SWING_FORWARD:
        set_phase(s, SWING_RESOLVE);
        break;
    case SWING_PAST_BOTTOM:
        if (p->ms-s->phase_ms > SWING_RELEASE_WINDOW_MS)
            return reject(s, SWING_REJECT_TOO_LATE);
        return resolve(s, profile, height);
    case SWING_READY:
    case SWING_RESOLVE:
    case SWING_SPENT: break;
    }
    return (SwingResult){0};
}

SwingResult swing_feed(Swing *s, const SwingSample *in, const SwingProfile *profile, int height) {
    SwingResult bridged = {0};
    if (s->phase == SWING_SPENT) return bridged;
    SwingSample p = *in;
    double magnitude = norm(p.g);
    bool motion = p.accel_valid && isfinite(magnitude) && magnitude >= 0.1;
    if (s->have_time && p.ms <= s->last_ms) p.ms = s->last_ms+1;
    if (s->have_time) {
        uint64_t gap = p.ms-s->last_ms;
        if (gap > SWING_RESET_GAP_MS) {
            if (swing_armed(s) && (s->phase != SWING_GRIP || !p.b)) {
                swing_report_gap(s);
                bridged.reject = SWING_REJECT_REPORT_GAP;
            } else {
                s->have_time = s->have_g = s->pitch_static = false;
                s->still_ms = 0;
                s->onset = false;
            }
        } else if (gap > SWING_GAP_MS && gap <= SWING_BRIDGE_MS && motion && s->have_g) {
            unsigned steps = (unsigned)((gap+SWING_BRIDGE_STEP_MS-1)/SWING_BRIDGE_STEP_MS);
            uint64_t t0 = s->last_ms;
            SwingVector g0 = s->last_g;
            bool clipped = s->last_clipped || p.clipped;
            for (unsigned k = 1; k < steps; ++k) {
                double u = (double)k/steps;
                SwingSample q = {.ms=t0+(uint64_t)llround(u*(double)gap),
                    .g={g0.x+(p.g.x-g0.x)*u, g0.y+(p.g.y-g0.y)*u, g0.z+(p.g.z-g0.z)*u},
                    .b=s->previous_b, .clipped=clipped, .accel_valid=true};
                SwingResult r = advance(s, &q, true, norm(q.g), profile, height);
                if (r.thrown) return r;
                if (r.reject != SWING_REJECT_NONE) bridged = r;
            }
            ++s->bridged;
        }
    }
    SwingResult r = advance(s, &p, motion, magnitude, profile, height);
    return r.thrown || r.reject != SWING_REJECT_NONE ? r : bridged;
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
