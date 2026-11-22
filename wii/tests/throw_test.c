/* Wii 0.1.3: Bowling-style throw traces (WII-0.1.3 spec §4 a-f, plus gap rules).
 *
 * Every trace is synthetic but physical: a shoulder-pivot arm model (remote Y tangent,
 * Z radial; see input_test.c pendulum()) with hand tremor, Gaussian sensor noise and
 * 10-bit quantization (104 counts per g around 512, the libogc calibration used in
 * wpad_test.c). "Remote up" is the arm raised forward so the remote's +Y axis reads
 * gravity, as in Wii Sports Bowling's starting pose.
 *
 * The traces go through the real Input layer (input_feed), the way the game feeds them,
 * starting the moment the turn opens: no settling period is granted before B goes down. */
#include "input.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static unsigned tests, failures;
/* `throw-test settled` grants 700 ms of still, B-up reports before each turn: a diagnostic that
 * isolates the root causes 0.1.2 has beyond its settle-before-every-throw gate. */
static bool settled;
#define CHECK(cond, ...) do { if (!(cond)) { ++failures; printf("  FAIL %s:%d: ", __FILE__, __LINE__); \
    printf(__VA_ARGS__); printf("\n"); } } while (0)
#define TEST(fn) do { unsigned before=failures; fn(); ++tests; \
    printf("%s %u - %s\n", failures==before ? "ok" : "not ok", tests, #fn); } while (0)

#include "bowling_trace.h"

/* ---------- harness ---------- */
typedef struct { InputEvent events[64]; unsigned count, throws; SwingResult toss; } Events;
static void sink(void *user, const InputEvent *e) {
    Events *ev=user; if (ev->count<64) ev->events[ev->count++]=*e;
    if (e->action==INPUT_THROW) { ++ev->throws; ev->toss=e->toss; }
}
typedef struct {
    unsigned throws, samples;
    SwingResult toss;
    uint64_t press_ms, release_ms, grip_delay_ms, throw_at_ms;
    bool gripped;
    uint64_t backswing_ms, forward_ms;
    double peak;
    bool ready;
} Outcome;
static void begin(Input *in, Events *ev) {
    *ev=(Events){0};
    input_init(in,sink,ev);
    input_connection(in,0,true);
    input_start_match(in); /* the turn opens now; no settle time */
    if (settled) for (uint64_t t=300;t<1000;t+=10) {
        InputSample p={.motion={.ms=t,.g={0,sin(80*SWING_PI/180),cos(80*SWING_PI/180)},.accel_valid=true},
            .buttons_valid=true};
        input_feed(in,0,&p);
    }
}
static void feed(Input *in, uint64_t ms, SwingVector g, bool b, bool clipped, unsigned dropped) {
    InputSample p={.motion={.ms=ms,.g=g,.b=b,.clipped=clipped,.accel_valid=true,.dropped=dropped},
        .buttons=b ? INPUT_B : 0,.buttons_valid=true};
    input_feed(in,0,&p);
}
static Outcome run_into(Input *in, Events *ev, const Trace *t, uint64_t base, unsigned start_ms) {
    Outcome o={0};
    rng_state=t->seed;
    in->slots[in->active_slot].profile.left_handed=t->left;
    uint64_t release=0, last=0;
    bool had_b=false, first=true;
    unsigned period=1000/t->rate_hz;
    unsigned throws_before=ev->throws;
    bool drop_flag=false;
    for (unsigned k=start_ms/period;;++k) {
        double ms=k*period+(t->jitter_ms ? (uniform()*2-1)*t->jitter_ms : 0);
        if (ms<0) ms=0;
        Pose p;
        if (!pose(t,ms,&p,&release)) break;
        if (t->gaps && in_gap(ms,release)) { drop_flag=true; continue; }
        uint64_t at=base+(uint64_t)llround(ms);
        if (!first && at<=last) at=last+1;
        first=false; last=at;
        SwingVector f=specific_force(&p,t->left);
        bool clipped=false;
        unsigned x=raw_axis(f.x,&clipped);
        unsigned y=raw_tilt_axis(f.y,&clipped);
        unsigned z=raw_tilt_axis(f.z,&clipped);
        SwingVector g={(x-512.0)/104,(y-512.0)/104,(z-512.0)/104};
        feed(in,at,g,p.b,clipped,drop_flag ? 1 : 0);
        drop_flag=false;
        const char *phase=swing_phase_name(&in->detector);
        if (!o.backswing_ms && !strcmp(phase,"BACKSWING")) o.backswing_ms=at-base;
        if (!o.forward_ms && !strcmp(phase,"FORWARD")) o.forward_ms=at-base;
        o.peak=fmax(o.peak,in->detector.peak);
        ++o.samples;
        if (p.b && !had_b) {
            o.press_ms=at;
            o.gripped=swing_armed(&in->detector);
            o.grip_delay_ms=o.gripped ? 0 : 9999;
        }
        if (!p.b && had_b && !o.release_ms) o.release_ms=at;
        had_b=p.b;
        if (ev->throws>throws_before && !o.throw_at_ms) o.throw_at_ms=at;
    }
    o.ready=in->detector.phase==SWING_READY;
    o.throws=ev->throws-throws_before;
    o.toss=ev->toss;
    return o;
}
static Outcome run(const Trace *t) {
    Input in; Events ev; begin(&in,&ev);
    return run_into(&in,&ev,t,1000,0);
}
static void report(const char *name, const Outcome *o) {
    printf("  %-34s throws %u power %.3f speed %.2f aim %+.3f spin %+.3f grip %s release->throw %lld ms\n",
           name,o->throws,o->toss.power,o->toss.speed,o->toss.aim,o->toss.spin,
           o->gripped ? "0 ms" : "NO",o->throw_at_ms ? (long long)(o->throw_at_ms-o->release_ms) : -1LL);
}
static void full_throw_checks(const char *name, const Outcome *o) {
    report(name,o);
    CHECK(o->gripped,"%s: B down did not grip on the same report",name);
    CHECK(o->throws==1,"%s: %u throws",name,o->throws);
    CHECK(o->throws!=1 || o->toss.speed>=swing_speed(.3),"%s: speed %.3f",name,o->toss.speed);
}

/* (a) B pressed while the remote is still moving into the start pose. */
static void a_press_while_moving(void) {
    Trace t=bowling(); t.press_moving=true;
    Outcome o=run(&t);
    full_throw_checks("(a) press while moving",&o);
}
/* (b) Remote-up start, full back and forward swing, B up mid-forward. */
static void b_bowling_swing(void) {
    Trace t=bowling();
    Outcome o=run(&t);
    full_throw_checks("(b) remote-up bowling swing",&o);
    CHECK(o.throws!=1 || o.throw_at_ms==o.release_ms,"(b): throw waited %lld ms after B up",
          (long long)(o.throw_at_ms-o.release_ms));
    const double releases[]={-5,0,20,35};
    for (unsigned k=0;k<4;++k) {
        t=bowling(); t.release_deg=releases[k]; t.seed=11+k;
        o=run(&t);
        char name[48]; snprintf(name,sizeof name,"(b) release at %+.0f deg",releases[k]);
        full_throw_checks(name,&o);
    }
}
/* (c) (b) with gaps and jitter at 100 Hz and 200 Hz. */
static void c_gaps_and_jitter(void) {
    const unsigned rates[]={100,200};
    for (unsigned r=0;r<2;++r) for (unsigned gaps=0;gaps<2;++gaps) {
        Trace t=bowling(); t.rate_hz=rates[r]; t.jitter_ms=rates[r]==100 ? 4 : 2;
        t.gaps=gaps; t.seed=21+r*2+gaps;
        Outcome o=run(&t);
        char name[48]; snprintf(name,sizeof name,"(c) %u Hz jitter%s",rates[r],gaps ? " + 30-60 ms gaps" : "");
        full_throw_checks(name,&o);
    }
}
/* (d) Slow, gentle swing: still a throw, softer than (b). */
static void d_gentle_swing(void) {
    Trace t=bowling(); t.back_deg=25; t.fwd_deg=30; t.back_s=1.1; t.fwd_s=.7; t.release_deg=5; t.seed=31;
    Outcome o=run(&t);
    Trace full=bowling(); Outcome strong=run(&full);
    report("(d) gentle swing",&o);
    CHECK(o.gripped,"(d): no grip");
    CHECK(o.throws==1,"(d): %u throws",o.throws);
    CHECK(o.throws!=1 || (o.toss.power>0 && o.toss.power<strong.toss.power),
          "(d): power %.3f vs full %.3f",o.toss.power,strong.toss.power);
}
/* (e) A B tap with tremor cancels and the next swing can throw. */
static void e_tap_cancels(void) {
    Input in; Events ev; begin(&in,&ev);
    Trace t=bowling(); t.tap=true; t.seed=41;
    Outcome o=run_into(&in,&ev,&t,1000,0);
    report("(e) B tap, no swing",&o);
    CHECK(o.gripped && o.throws==0 && in.detector.phase==SWING_READY,"(e): tap did not cancel");
    t=bowling();
    o=run_into(&in,&ev,&t,2500,0);
    full_throw_checks("(e) next swing",&o);
}
/* (f) Two throws back to back: the game takes the first bag, the next turn opens, and the
 * player is already swinging again (B pressed during the flight, and B pressed the instant
 * the turn opens). */
static void f_back_to_back(void) {
    for (unsigned variant=0;variant<2;++variant) {
        Input in; Events ev; begin(&in,&ev);
        Trace t=bowling();
        Outcome first=run_into(&in,&ev,&t,1000,0);
        /* Bag flight: the game disables throw input, then opens the next turn. */
        input_set_context(&in,INPUT_DISABLED);
        uint64_t now=1000+2200;
        for (unsigned k=0;k<60;++k,now+=10)
            feed(&in,now,(SwingVector){0,sin(80*SWING_PI/180),cos(80*SWING_PI/180)},variant==0 && k>40,false,0);
        input_next_turn(&in,0);
        input_set_context(&in,INPUT_PLAY);
        Trace second=bowling(); second.seed=52;
        CHECK(in.b_held[0]==(variant==0),"(f): flight B state did not carry into the turn");
        Outcome o=run_into(&in,&ev,&second,now-T_PRESS,T_PRESS);
        char name[48]; snprintf(name,sizeof name,"(f) second throw, B %s",variant==0 ? "held over" : "fresh press");
        report("(f) first throw",&first);
        report(name,&o);
        CHECK(first.throws==1,"(f): first throw %u",first.throws);
        CHECK(o.gripped && o.throws==1,"%s: immediate grip %d, throws %u",name,o.gripped,o.throws);
        CHECK(o.throws!=1 || o.toss.speed>=swing_speed(.3),"%s: speed %.3f",name,o.toss.speed);
    }
}

/* Gap rules (spec §2.5): one late or dropped report never resets an armed swing. */
static void single_late_or_dropped_sample_keeps_grip(void) {
    const unsigned gaps[]={26,40,60,100};
    for (unsigned k=0;k<4;++k) {
        Input in; Events ev; begin(&in,&ev);
        SwingVector up={0,1,0};
        feed(&in,1000,up,false,false,0);
        feed(&in,1010,up,true,false,0);
        CHECK(swing_armed(&in.detector),"gap %u: no grip",gaps[k]);
        feed(&in,1020,up,true,false,0);
        feed(&in,1020+gaps[k],up,true,false,1);
        CHECK(swing_armed(&in.detector),"gap %u ms reset the armed swing",gaps[k]);
        feed(&in,1030+gaps[k],up,false,false,0);
        CHECK(ev.throws==0 && in.detector.phase==SWING_READY,"gap %u ms: release after the gap gave %u throws",gaps[k],ev.throws);
    }
    /* A report with an accel error inside the swing does not reset either. */
    Input in; Events ev; begin(&in,&ev);
    feed(&in,990,(SwingVector){0,1,0},false,false,0);
    feed(&in,1000,(SwingVector){0,1,0},true,false,0);
    CHECK(swing_armed(&in.detector),"no grip before the invalid accel report");
    InputSample bad={.motion={.ms=1010,.b=true,.accel_valid=false},.buttons=INPUT_B,.buttons_valid=true};
    input_feed(&in,0,&bad);
    CHECK(swing_armed(&in.detector),"invalid accel report reset the armed swing");
}
static void long_gap_never_swallows_release(void) {
    Input in; Events ev; begin(&in,&ev);
    SwingVector up={0,1,0};
    feed(&in,990,up,false,false,0);
    feed(&in,1000,up,true,false,0);
    feed(&in,1010,up,true,false,0);
    feed(&in,1400,up,true,false,0); /* 390 ms stall with B still held */
    feed(&in,1410,up,false,false,0);
    CHECK(ev.throws==0 && in.detector.phase==SWING_READY,"release after a 390 ms stall gave %u throws",ev.throws);
}
static void history_overflow_drops_oldest(void) {
    Input in; Events ev; begin(&in,&ev);
    SwingVector up={0,1,0};
    feed(&in,990,up,false,false,0);
    feed(&in,1000,up,true,false,0);
    /* 1 kHz bursts with near-identical timestamps overflow the 192-sample ring. */
    uint64_t t=1001;
    for (unsigned k=0;k<SWING_HISTORY*2;++k) feed(&in,t++,up,true,false,0);
    CHECK(swing_armed(&in.detector),"history overflow reset the armed swing");
    feed(&in,t+5,up,false,false,0);
    CHECK(ev.throws==0 && in.detector.phase==SWING_READY,"release after overflow gave %u throws",ev.throws);
}
/* Aim and curl survive the new path: lateral swing aims, wrist twist curls, mirrored for lefties. */
static void aim_spin_and_hand_on_bowling_swing(void) {
    Trace t=bowling(); Outcome straight=run(&t);
    t=bowling(); t.lateral_g=.35; Outcome right=run(&t);
    t=bowling(); t.lateral_g=.35; t.left=true; Outcome lefty=run(&t);
    t=bowling(); t.twist_deg=35; t.release_deg=0; Outcome twist=run(&t);
    report("straight",&straight); report("lateral right",&right); report("lateral right, left hand",&lefty);
    report("twist",&twist);
    CHECK(straight.throws==1 && right.throws==1 && lefty.throws==1 && twist.throws==1,"aim/spin traces must throw");
    if (straight.throws==1 && right.throws==1 && lefty.throws==1) {
        CHECK(right.toss.aim>straight.toss.aim+.01,"lateral swing should aim right (%.3f vs %.3f)",right.toss.aim,straight.toss.aim);
        /* A left-handed mirror image of the same swing aims the same way. */
        CHECK(fabs(lefty.toss.aim-right.toss.aim)<.02,"left hand mirror aim %.3f vs %.3f",lefty.toss.aim,right.toss.aim);
    }
}

/* Power follows forward swing speed, from a hanging start and from the remote-up start. */
static void power_follows_forward_speed(void) {
    const double durations[]={.9,.75,.6,.5,.4};
    for (int start=0;start<2;++start) {
        double last=0;
        for (unsigned k=0;k<5;++k) {
            Trace t=bowling(); t.start_deg=start ? 80 : 0; t.back_deg=35; t.fwd_deg=40;
            t.back_s=.7; t.fwd_s=durations[k]; t.release_deg=5;
            Outcome o=run(&t);
            printf("  start %2.0f deg, forward %.2f s: power %.3f%s\n",t.start_deg,t.fwd_s,o.toss.power,
                   o.throws ? "" : " (cancelled)");
            CHECK(o.throws==1 && o.toss.power>last,"forward %.2f s: power %.3f not above %.3f",t.fwd_s,o.toss.power,last);
            last=o.toss.power;
        }
        CHECK(last>.39,"a quick forward swing should produce at least 0.39 power (%.3f)",last);
    }
}
/* B used to leave the pause menu must not grip on the way back into play. */
static void b_from_pause_menu_does_not_lob(void) {
    Input in; Events ev; begin(&in,&ev);
    SwingVector up={0,1,0};
    feed(&in,1000,up,false,false,0);
    InputSample home={.motion={.ms=1010,.g=up,.accel_valid=true},.buttons=INPUT_HOME,.buttons_valid=true};
    input_feed(&in,0,&home);
    CHECK(in.paused,"HOME did not pause");
    input_set_context(&in,INPUT_MENU);
    feed(&in,1020,up,true,false,0);          /* B = BACK in the pause menu */
    input_set_context(&in,INPUT_PLAY);       /* the game resumes the play context */
    CHECK(!input_resume(&in),"resume must wait for B up");
    feed(&in,1030,up,true,false,0);
    CHECK(!swing_armed(&in.detector),"B held from the menu gripped");
    feed(&in,1040,up,false,false,0);
    CHECK(input_resume(&in) && ev.throws==0,"letting go of the menu B threw %u bags",ev.throws);
    feed(&in,1050,up,true,false,0);
    CHECK(swing_armed(&in.detector),"a fresh press after resuming must grip at once");
}
/* Timing against the model with synthetic sensor noise, quantization and report jitter. */
static void phase_timing(void) {
    const struct { double start, back, fwd, back_s, fwd_s; bool moving; } cases[]={
        {80,55,60,.55,.45,false},
        {80,55,60,.55,.45,true},
        {0,45,60,.50,.50,false},
        {80,55,60,.40,.65,false},
        {0,45,60,.50,.50*sqrt(105.0/45),false},
    };
    double back_sum=0, forward_sum=0, back_worst=0, forward_worst=0;
    unsigned back_count=0, forward_count=0;
    for (unsigned i=0;i<sizeof cases/sizeof cases[0];++i) {
        Trace t=bowling();
        t.start_deg=cases[i].start; t.back_deg=cases[i].back; t.fwd_deg=cases[i].fwd;
        t.back_s=cases[i].back_s; t.fwd_s=cases[i].fwd_s; t.press_moving=cases[i].moving;
        t.seed=7+i; t.jitter_ms=i%2 ? 2 : 0;
        Outcome o=run(&t);
        double back_error=o.backswing_ms ? (double)o.backswing_ms-backswing_start_ms() : NAN;
        double forward_error=o.forward_ms ? (double)o.forward_ms-forward_start_ms(&t) : NAN;
        printf("  phase fixture %u: BACKSWING %+.0f ms, FORWARD %+.0f ms\n",i+1,back_error,forward_error);
        if (isfinite(back_error)) {
            back_sum+=fabs(back_error); back_worst=fmax(back_worst,fabs(back_error)); ++back_count;
        }
        if (isfinite(forward_error)) {
            forward_sum+=fabs(forward_error); forward_worst=fmax(forward_worst,fabs(forward_error)); ++forward_count;
        }
        CHECK(back_error>=0 && back_error<=30,"fixture %u: backswing outside 30 ms",i+1);
        CHECK(forward_error>=0 && forward_error<=50,"fixture %u: forward outside 50 ms",i+1);
    }
    printf("  timing mean/worst absolute errors: BACKSWING %.1f/%.0f ms (%u), FORWARD %.1f/%.0f ms (%u)\n",
           back_sum/back_count,back_worst,back_count,forward_sum/forward_count,forward_worst,forward_count);
}
static void subthreshold_and_backswing_cancel(void) {
    Input in; Events ev; begin(&in,&ev);
    Trace t=bowling();
    t.start_deg=0; t.back_deg=2; t.fwd_deg=2; t.release_deg=1;
    Outcome tiny=run_into(&in,&ev,&t,1000,0);
    CHECK(tiny.gripped && tiny.ready && tiny.throws==0 && tiny.peak==0,"tiny wiggle did not cancel");
    t=bowling(); t.back_only=true;
    Outcome back=run_into(&in,&ev,&t,3500,0);
    CHECK(back.gripped && back.ready && back.throws==0 && back.peak==0 && !back.forward_ms,
          "backswing-only was treated as forward (peak %.3f m/s)",back.peak);
    t.back_only=false;
    Outcome full=run_into(&in,&ev,&t,6000,0);
    full_throw_checks("after two cancelled swings",&full);
}

int main(int argc, char **argv) {
    settled=argc==2 && strcmp(argv[1],"settled")==0;
    TEST(a_press_while_moving);
    TEST(b_bowling_swing);
    TEST(c_gaps_and_jitter);
    TEST(d_gentle_swing);
    TEST(e_tap_cancels);
    TEST(f_back_to_back);
    TEST(single_late_or_dropped_sample_keeps_grip);
    TEST(long_gap_never_swallows_release);
    TEST(history_overflow_drops_oldest);
    TEST(aim_spin_and_hand_on_bowling_swing);
    TEST(b_from_pause_menu_does_not_lob);
    TEST(power_follows_forward_speed);
    TEST(phase_timing);
    TEST(subthreshold_and_backswing_cancel);
    printf("%u throw-trace tests, %u failed checks\n",tests,failures);
    return failures ? 1 : 0;
}
