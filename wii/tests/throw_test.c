/* Replay noisy, quantized physical swings through the game's input route. */
#include "input.h"
#include "bowling_trace.h"
#include <math.h>
#include <stdio.h>

static unsigned tests, failures;
#define CHECK(cond, ...) do { if (!(cond)) { ++failures; printf("  FAIL %d: ", __LINE__); \
    printf(__VA_ARGS__); putchar('\n'); } } while (0)
#define TEST(fn) do { unsigned before=failures; fn(); ++tests; \
    printf("%s %u - %s\n", failures==before ? "ok" : "not ok", tests, #fn); } while (0)

typedef struct {
    unsigned throws, regressions;
    SwingResult toss;
    SwingReject reject;
    bool ready, gripped, have_axis, resolved;
    double ny, nz, backswing_peak, fitted_speed;
    uint64_t release_ms, throw_ms, back_ms, forward_ms, bottom_ms;
} Outcome;

static void sink(void *user, const InputEvent *event) {
    Outcome *out=user;
    if (event->action==INPUT_THROW) {
        ++out->throws;
        out->toss=event->toss;
    }
}

static void begin(Input *in, Outcome *out) {
    input_init(in,sink,out);
    input_connection(in,0,true);
    input_start_match(in);
}

static void feed(Input *in, uint64_t ms, SwingVector g, bool b, unsigned dropped) {
    InputSample sample={.motion={.ms=ms,.g=g,.accel_valid=true,.dropped=dropped},
        .buttons=b ? INPUT_B : 0,.buttons_valid=true};
    input_feed(in,0,&sample);
}

static void replay_into(Input *in, Outcome *out, const BowlingTrace *trace, unsigned rate,
                        uint32_t seed, uint64_t base, bool gaps, bool duplicate_reports) {
    bool previous_b=false, dropped=false;
    uint64_t last=base;
    SwingPhase previous_phase=SWING_READY;
    SwingVector previous_g={0,-1,0};
    double release=bowling_release(trace);
    double end=fmax(bowling_forward(trace)+trace->forward_s+1,isfinite(release) ? release+.25 : 0);
    for (unsigned report=0;report<end*rate;++report) {
        double nominal=(double)report/rate;
        double s=fmax(0,nominal+(.004*bowling_uniform(&seed)-.002));
        if (gaps && ((s>.480 && s<.525) || (s>.935 && s<.980) ||
                     (s>1.430 && s<1.485) || (s>=release && s<release+.035))) {
            dropped=true;
            continue;
        }
        uint64_t at=base+(uint64_t)llround(s*1000);
        if (report && at<=last) at=last+1;
        bool b=bowling_button(trace,s);
        SwingVector g=bowling_quantized(bowling_force(trace,s),&seed);
        if (duplicate_reports && report%7==1) g=previous_g;
        unsigned throws_before=out->throws;
        feed(in,at,g,b,dropped);
        dropped=false;
        Swing *detector=&in->detector;
        SwingPhase phase=detector->phase;
        if (b && !previous_b) out->gripped=swing_armed(detector);
        if (!b && previous_b) out->release_ms=at;
        if (out->throws>throws_before) out->throw_ms=at;
        if (phase==SWING_BACKSWING && !out->back_ms) out->back_ms=at-base;
        if (phase==SWING_FORWARD && !out->forward_ms) out->forward_ms=at-base;
        if (phase==SWING_RESOLVE) out->resolved=true;
        out->fitted_speed=detector->fitted_speed;
        if (detector->have_bottom) out->bottom_ms=detector->bottom_ms-base;
        if (detector->have_axis) {
            out->have_axis=true;
            out->ny=detector->ny;
            out->nz=detector->nz;
        }
        if (b && swing_armed(detector) && !out->forward_ms && phase!=SWING_FORWARD)
            out->backswing_peak=fmax(out->backswing_peak,detector->peak);
        if (b && previous_b && phase<previous_phase) ++out->regressions;
        previous_phase=phase;
        previous_b=b;
        previous_g=g;
        last=at;
    }
    out->ready=in->detector.phase==SWING_READY;
    out->reject=in->detector.last_reject;
}

static Outcome replay_height(const BowlingTrace *trace, unsigned rate, uint32_t seed,
                             bool gaps, bool duplicate_reports, int height) {
    Input in;
    Outcome out={0};
    begin(&in,&out);
    in.slots[0].height=height;
    replay_into(&in,&out,trace,rate,seed,1000,gaps,duplicate_reports);
    return out;
}

static Outcome replay(const BowlingTrace *trace, unsigned rate, uint32_t seed, bool gaps, bool duplicate_reports) {
    return replay_height(trace,rate,seed,gaps,duplicate_reports,1);
}

static void check_result(const char *name, const Outcome *out, SwingReject reject) {
    bool thrown=reject==SWING_REJECT_NONE;
    CHECK(out->gripped,"%s: B did not grip immediately",name);
    CHECK(out->throws==(unsigned)thrown,"%s: %u throws (reject %u)",name,out->throws,(unsigned)out->reject);
    if (thrown && out->throws==1) {
        CHECK(out->toss.release==SWING_B_UP && out->toss.release_ms==out->release_ms,
              "%s: release did not retain the B-up timestamp",name);
        CHECK(out->throw_ms>=out->release_ms && out->throw_ms-out->release_ms<=175,
              "%s: resolution took %llu ms",name,(unsigned long long)(out->throw_ms-out->release_ms));
        CHECK(out->toss.power>0 && out->toss.power<=SWING_POWER_CAP,"%s: invalid power %.3f",name,out->toss.power);
    }
    if (!thrown) {
        CHECK(out->ready,"%s: rejected but did not return to READY",name);
        CHECK(out->reject==reject,"%s: reject %u, expected %u",name,(unsigned)out->reject,(unsigned)reject);
    }
    CHECK(out->backswing_peak==0,"%s: backswing added power (%.3f m/s)",name,out->backswing_peak);
}

static void exact_static_seeds(void) {
    const SwingVector seeds[]={{0,-1,0},{0,0,1},{0,1,0},{1,0,0},{0,-.8660254,.5},{0,-.8660254,-.5}};
    const double pitches[]={90,0,-90,0,60,60};
    const double faces[]={90,0,-90,0,60,120};
    for (unsigned n=0;n<sizeof seeds/sizeof seeds[0];++n) {
        Input in;
        Outcome out={0};
        begin(&in,&out);
        for (uint64_t ms=1000;ms<3000;ms+=10) {
            bool b=ms>=2000 && ms<2500;
            feed(&in,ms,seeds[n],b,0);
            if (b) CHECK(in.detector.phase==SWING_GRIP,"seed %u left GRIP",n);
        }
        CHECK(in.detector.pitch_static && fabs(in.detector.pitch-pitches[n])<.5,
              "seed %u: expected static pitch %.1f, got %.1f",n,pitches[n],in.detector.pitch);
        if (n!=3) CHECK(fabs(in.detector.face_pitch-faces[n])<.5,
                        "seed %u: expected FACE %.1f, got %.1f",n,faces[n],in.detector.face_pitch);
        else CHECK(fabs(in.detector.roll-90)<.5,"right-roll seed: expected ROLL +90, got %.1f",in.detector.roll);
        CHECK(!in.detector.have_axis && out.throws==0 && in.detector.phase==SWING_READY &&
              in.detector.last_reject==SWING_REJECT_NO_SWING,"seed %u did not reject NO SWING",n);
    }
}

static void quasistatic_rotation_is_not_a_swing(void) {
    Input in;
    Outcome out={0};
    begin(&in,&out);
    uint32_t seed=7;
    double last_pitch=90;
    for (unsigned n=0;n<120;++n) {
        double seconds=n/60.0;
        double pitch=90*(1-bowling_fraction((seconds-.60)/.30));
        double angle=pitch*SWING_PI/180;
        SwingVector g={0,-sin(angle),cos(angle)};
        g=bowling_quantized(g,&seed);
        uint64_t ms=1000+(uint64_t)lround(seconds*1000);
        feed(&in,ms,g,seconds>=.50 && seconds<1.50,0);
        if (seconds>.65 && seconds<.85) {
            CHECK(in.detector.pitch_static && in.detector.pitch<last_pitch,
                  "quasi-static tip rotation did not report decreasing pitch");
            last_pitch=in.detector.pitch;
        }
    }
    CHECK(!out.throws && !in.detector.have_axis && in.detector.phase==SWING_READY &&
          in.detector.last_reject==SWING_REJECT_NO_SWING,"tip-up to flat rotation counted as a swing");
}

static void named_bowling_scenarios(void) {
    const struct {
        const char *name;
        double back, forward, back_s, forward_s, pause_s, pitch, release_deg, release_s;
        bool back_only, release_going_back, still;
        SwingReject reject;
    } cases[]={
        {"proper +15",     -50,70,.55,.45,0,0,15,-1,false,false,false,SWING_REJECT_NONE},
        {"proper +5",      -50,70,.55,.45,0,0,5,-1,false,false,false,SWING_REJECT_NONE},
        {"proper +40",     -50,70,.55,.45,0,0,40,-1,false,false,false,SWING_REJECT_NONE},
        {"slow forward",   -50,70,.55,.65,0,0,15,-1,false,false,false,SWING_REJECT_NONE},
        {"fast forward",   -50,70,.55,.32,0,0,15,-1,false,false,false,SWING_REJECT_NONE},
        {"fast backswing", -50,70,.40,.45,0,0,15,-1,false,false,false,SWING_REJECT_NONE},
        {"slow backswing", -50,70,.90,.45,0,0,15,-1,false,false,false,SWING_REJECT_NONE},
        {"top pause",      -50,70,.55,.45,.30,0,15,-1,false,false,false,SWING_REJECT_NONE},
        {"upright wrist",  -50,70,.55,.45,0,35,15,-1,false,false,false,SWING_REJECT_NONE},
        {"short back",     -25,60,.45,.40,0,0,12,-1,false,false,false,SWING_REJECT_NONE},
        {"leg going back", -50,70,.55,.45,0,0,-15,-1,true,true,false,SWING_REJECT_BACKSWING},
        {"bottom going back",-50,70,.55,.45,0,0,0,-1,true,true,false,SWING_REJECT_BACKSWING},
        {"at back apex",   -50,70,.55,.45,0,0,0,1.40,true,false,false,SWING_REJECT_BACKSWING},
        {"stop at leg",     0,70,.40,.45,0,0,0,1.30,true,false,false,SWING_REJECT_BACKSWING},
        {"leg on return",  -50,70,.55,.45,0,0,-20,-1,false,false,false,SWING_REJECT_BEFORE_BOTTOM},
        {"fast back early",-50,70,.40,.45,0,0,-20,-1,false,false,false,SWING_REJECT_BEFORE_BOTTOM},
        {"hard back early",-50,70,.30,.45,0,0,-20,-1,false,false,false,SWING_REJECT_BEFORE_BOTTOM},
        {"short back early",-25,60,.45,.40,0,0,-8,-1,false,false,false,SWING_REJECT_BEFORE_BOTTOM},
        {"still",          -50,70,.55,.45,0,0,0,1.80,false,false,true,SWING_REJECT_NO_SWING},
        {"late release",   -50,70,.55,.45,0,0,0,2.75,false,false,false,SWING_REJECT_TOO_LATE}
    };
    unsigned count=0;
    for (unsigned n=0;n<sizeof cases/sizeof cases[0];++n) {
        for (unsigned rate=60;rate<=100;rate+=40) {
            for (unsigned grip=0;grip<4;++grip) {
                BowlingTrace t=bowling_trace();
                t.back_deg=cases[n].back; t.forward_deg=cases[n].forward;
                t.back_s=cases[n].back_s; t.forward_s=cases[n].forward_s; t.pause_s=cases[n].pause_s;
                t.pitch_at_bottom=cases[n].pitch; t.release_deg=cases[n].release_deg; t.release_s=cases[n].release_s;
                t.back_only=cases[n].back_only; t.release_going_back=cases[n].release_going_back;
                t.still=cases[n].still; t.inline_grip=(grip&1)!=0; t.rolled=(grip&2)!=0;
                t.tremor_deg=.08;
                Outcome out=replay(&t,rate,7+n,false,false);
                char label[96];
                snprintf(label,sizeof label,"%s / %u Hz / %s%s",cases[n].name,rate,
                         t.inline_grip ? "in-line" : "tip-forward",t.rolled ? " rolled" : "");
                check_result(label,&out,cases[n].reject);
                if (n==0 && out.have_axis)
                    CHECK(t.inline_grip ? out.ny>.65 : fabs(out.nz)>.85,"%s: learned axis (%.3f, %.3f)",label,out.ny,out.nz);
                ++count;
            }
        }
    }
    printf("  %u physical scenario replays\n",count);
}

static void forward_power_and_clip_reconstruction(void) {
    const double back_times[]={.4,.55,.7,.9,1};
    const double forward_times[]={.75,.65,.60,.55,.50,.46};
    BowlingTrace t=bowling_trace();
    double min_power=1,max_power=0,previous_speed=0;
    /* Use the slower, unclipped portion of the curve to isolate backswing power. */
    t.forward_s=.55;
    for (unsigned n=0;n<sizeof back_times/sizeof back_times[0];++n) {
        t.back_s=back_times[n];
        Outcome out=replay(&t,100,7,false,false);
        check_result("fixed forward speed",&out,SWING_REJECT_NONE);
        min_power=fmin(min_power,out.toss.power);
        max_power=fmax(max_power,out.toss.power);
    }
    CHECK(min_power>0 && (max_power-min_power)/max_power<=.05,
          "backswing alone varied power from %.3f to %.3f",min_power,max_power);
    t.back_s=.55;
    t.release_deg=40; /* Leave room for both unclipped shoulders before B-up. */
    for (unsigned n=0;n<sizeof forward_times/sizeof forward_times[0];++n) {
        t.forward_s=forward_times[n];
        Outcome out=replay(&t,100,7,false,false);
        check_result("increasing forward speed",&out,SWING_REJECT_NONE);
        CHECK(out.toss.speed>previous_speed,"forward %.2f s: speed %.3f <= %.3f",t.forward_s,out.toss.speed,previous_speed);
        previous_speed=out.toss.speed;
    }
    t.forward_s=.46;
    Outcome clipped=replay(&t,100,7,false,false);
    check_result("clipped forward lobe",&clipped,SWING_REJECT_NONE);
    CHECK(clipped.toss.clipped && clipped.toss.speed>swing_speed(2.55),
          "clipped shoulders did not recover speed above the assumed rail (%.3f m/s)",clipped.toss.speed);
    t.release_deg=15;
    Outcome deferred=replay(&t,100,7,false,false);
    check_result("clipped lobe resolved after B-up",&deferred,SWING_REJECT_NONE);
    CHECK(deferred.resolved && deferred.toss.clipped && deferred.fitted_speed==0,
          "deferred fit used shoulders after B-up (%.3f m/s)",deferred.fitted_speed);
}

static void throw_type_power_and_cap(void) {
    const double previous_scales[]={1.30,1.20,1.0/.90};
    const double effort[]={1.30,1.15,.90};
    BowlingTrace mid=bowling_trace(), strong=bowling_trace();
    mid.forward_s=.75;
    mid.release_deg=40;
    strong.back_deg=-90;
    strong.forward_deg=90;
    strong.forward_s=.45;
    strong.release_deg=75;
    strong.inline_grip=true;
    /* Center the clipped crest differently relative to the 60 Hz report times. */
    BowlingTrace strong_60=strong;
    strong_60.back_deg=-88;
    strong_60.forward_deg=89;
    strong_60.back_s=.578;
    strong_60.forward_s=.438;
    strong_60.release_deg=79;
    strong_60.pitch_at_bottom=-5;
    strong_60.rolled=true;
    for (int height=0;height<3;++height) {
        Outcome toss=replay_height(&mid,100,7,false,false,height);
        check_result("unclipped mid-range power",&toss,SWING_REJECT_NONE);
        double old_power=swing_power(toss.toss.speed,SWING_DEFAULT_FULL_SPEED*previous_scales[height]);
        CHECK(!toss.toss.clipped && old_power>0 && old_power<SWING_POWER_CAP &&
              toss.toss.power<SWING_POWER_CAP && fabs(toss.toss.power-old_power/effort[height])<1e-12,
              "type %d: mid-range power %.3f vs 0.1.8 power %.3f",height,toss.toss.power,old_power);

        for (unsigned rate=60;rate<=100;rate+=40) {
            Outcome cap=replay_height(rate==60 ? &strong_60 : &strong,rate,rate==60 ? 33 : 7,false,false,height);
            check_result("strong clipped forward swing",&cap,SWING_REJECT_NONE);
            double uncapped=SWING_POWER_GAIN*cap.toss.speed/(SWING_DEFAULT_FULL_SPEED*swing_type_scale(height));
            CHECK(cap.toss.clipped && cap.fitted_speed>swing_speed(SWING_RAIL_G-1) &&
                  uncapped>SWING_POWER_CAP && cap.toss.power==SWING_POWER_CAP,
                  "type %d at %u Hz: fitted speed %.3f m/s, uncapped power %.3f, result %.3f",
                  height,rate,cap.fitted_speed,uncapped,cap.toss.power);
        }
    }
}

static void crooked_backswing_does_not_aim(void) {
    BowlingTrace trace=bowling_trace();
    trace.backswing_lateral_g=1.25;
    for (unsigned rate=60;rate<=100;rate+=40) {
        trace.inline_grip=false;
        for (unsigned grip=0;grip<2;++grip) {
            Outcome out=replay(&trace,rate,7,false,false);
            check_result("crooked backswing, straight forward",&out,SWING_REJECT_NONE);
            CHECK(out.toss.aim==0,"backswing biased aim by %.4f at %u Hz, grip %u",out.toss.aim,rate,grip);
            trace.inline_grip=true;
        }
    }
}

static void low_radial_load_rejects_as_too_weak(void) {
    BowlingTrace trace=bowling_trace();
    trace.back_deg=-35; trace.forward_deg=35; trace.forward_s=.88;
    Outcome out=replay(&trace,100,7,false,false);
    check_result("weak forward radial lobe",&out,SWING_REJECT_TOO_WEAK);
    CHECK(out.bottom_ms>0,"weak lobe did not get a bottom marker");
    trace.forward_s=.98;
    out=replay(&trace,100,7,false,false);
    check_result("forward crest below 1.30 g",&out,SWING_REJECT_TOO_WEAK);
    CHECK(out.resolved && !out.bottom_ms,"subthreshold lobe did not use the deferred reject path");
}

static void release_during_turn_confirmation(void) {
    BowlingTrace t=bowling_trace();
    Input in; Outcome out={0};
    begin(&in,&out);
    unsigned ms=0;
    for (;ms<1350;ms+=10) {
        double seconds=ms/1000.0;
        feed(&in,1000+ms,bowling_force(&t,seconds),bowling_button(&t,seconds),0);
    }
    CHECK(in.detector.phase==SWING_TURN && in.detector.have_axis && !in.detector.turn_rise,
          "fixture did not reach the turnaround minimum");
    feed(&in,1000+ms,bowling_force(&t,ms/1000.0),false,0);
    CHECK(!out.throws && in.detector.phase==SWING_READY && !in.detector.have_axis &&
          in.detector.last_reject==SWING_REJECT_BEFORE_BOTTOM,
          "B-up on the first turnaround rise should reject as before bottom and clear the axis");
}

static void sustained_noise_and_short_gaps(void) {
    BowlingTrace t=bowling_trace(); t.tremor_deg=.08;
    for (unsigned rate=60;rate<=100;rate+=40) {
        Outcome out=replay(&t,rate,15,true,true);
        check_result("30–60 ms gaps, noise, jitter and repeated reports",&out,SWING_REJECT_NONE);
        CHECK(!out.regressions,"phase regressed %u times while B stayed down",out.regressions);
        Outcome phase=replay(&t,rate,7,false,true);
        CHECK(phase.back_ms>0 && phase.forward_ms>phase.back_ms,"missing motion phases at %u Hz",rate);
        CHECK(out.bottom_ms>out.forward_ms && out.bottom_ms<=out.release_ms-1000+SWING_BOTTOM_TOL_MS,
              "bottom estimate out of order at %u Hz",rate);
    }
    t.still=true; t.tremor_deg=.85; t.release_s=1.8;
    Outcome wiggle=replay(&t,100,7,false,true);
    CHECK(wiggle.ready && wiggle.throws==0 && !wiggle.forward_ms && !wiggle.regressions,"wiggle flapped phases or threw");
}

static void release_gap_and_overflow(void) {
    const SwingVector up={0,-1,0};
    Input in; Outcome out={0};
    begin(&in,&out);
    feed(&in,990,up,false,0);
    feed(&in,1000,up,true,0);
    feed(&in,1010,(SwingVector){0,-1,1},true,0);
    feed(&in,1400,up,true,0); /* 390 ms gap while still held */
    feed(&in,1410,up,false,0);
    CHECK(out.throws==0 && in.detector.phase==SWING_READY &&
          in.detector.last_reject==SWING_REJECT_REPORT_GAP,"390 ms report gap lost the cancel reason");
    feed(&in,1420,up,true,0);
    for (uint64_t ms=1421;ms<1820;++ms) feed(&in,ms,up,true,0);
    CHECK(swing_armed(&in.detector),"high-rate history overflow reset the grip");
    feed(&in,1820,up,false,0);
    CHECK(in.detector.phase==SWING_READY && in.detector.last_reject==SWING_REJECT_NO_SWING,
          "high-rate stationary samples did not reject NO SWING");
}

static void retry_and_handed_controls(void) {
    Input in; Outcome out={0};
    begin(&in,&out);
    BowlingTrace t=bowling_trace(); t.release_deg=-20;
    replay_into(&in,&out,&t,100,7,1000,false,false);
    check_result("early first toss",&out,SWING_REJECT_BEFORE_BOTTOM);
    out=(Outcome){0}; t=bowling_trace(); t.lateral_g=.4; t.twist_deg=40;
    replay_into(&in,&out,&t,100,7,5000,false,false);
    check_result("retry",&out,SWING_REJECT_NONE);
    CHECK(out.toss.aim<0 && fabs(out.toss.spin)>.2,"lateral and twist inputs missing (%.3f, %.3f)",out.toss.aim,out.toss.spin);
    SwingResult right=out.toss;
    input_next_turn(&in,0);
    in.slots[0].profile.left_handed=true;
    out=(Outcome){0};
    replay_into(&in,&out,&t,100,7,9000,false,false);
    check_result("same physical controls, left hand",&out,SWING_REJECT_NONE);
    CHECK(fabs(right.aim-out.toss.aim)<1e-12 && fabs(right.spin+out.toss.spin)<1e-12 &&
          fabs(right.power-out.toss.power)<1e-12,"changing hands changed aim/power or failed to mirror curl");
    swing_cancel(&in.detector,true);
    CHECK(in.detector.phase==SWING_READY && !in.detector.have_axis && !in.detector.observer_valid,
          "cancel kept the learned axis or arm estimate");
    t.rolled=true;
    out=replay(&t,100,7,false,false);
    check_result("same trace rolled 180 degrees",&out,SWING_REJECT_NONE);
    CHECK(out.toss.aim>0 && right.spin*out.toss.spin<0,
          "180 degree roll did not reverse lateral and twist inputs");
}

static void pending_release_expires_without_reports(void) {
    BowlingTrace trace=bowling_trace();
    trace.release_deg=-20;
    Input in;
    Outcome out={0};
    begin(&in,&out);
    uint64_t ms=0;
    for (unsigned tick=0;tick<2000;tick+=10) {
        double seconds=tick/1000.0;
        ms=1000+tick;
        feed(&in,ms,bowling_force(&trace,seconds),bowling_button(&trace,seconds),0);
        if (!bowling_button(&trace,seconds) && seconds>=bowling_release(&trace)) break;
    }
    CHECK(in.detector.phase==SWING_RESOLVE,"early B-up did not begin RESOLVE");
    swing_expire(&in.detector,ms+SWING_RESOLVE_MS-1);
    CHECK(in.detector.phase==SWING_RESOLVE,"RESOLVE ended before the timeout");
    swing_expire(&in.detector,ms+SWING_RESOLVE_MS);
    CHECK(out.throws==0 && in.detector.phase==SWING_READY &&
          in.detector.last_reject==SWING_REJECT_BEFORE_BOTTOM,
          "report silence did not resolve the early release");
}

static void randomized_releases(void) {
    uint32_t seed=2026;
    unsigned throws[2][4]={{0}};
    for (unsigned grip=0;grip<2;++grip) {
        for (unsigned kind=0;kind<4;++kind) {
            for (unsigned n=0;n<500;++n) {
                BowlingTrace t=bowling_trace();
                t.start_deg=25+40*bowling_uniform(&seed);
                t.back_deg=-40-20*bowling_uniform(&seed);
                t.forward_deg=40+40*bowling_uniform(&seed);
                t.back_s=.4+.3*bowling_uniform(&seed);
                t.forward_s=.35+.30*bowling_uniform(&seed);
                t.pitch_at_bottom=-15+40*bowling_uniform(&seed);
                t.tremor_deg=.08;
                t.rolled=n%2!=0; t.inline_grip=grip!=0;
                t.release_deg=kind==0 ? 5+35*bowling_uniform(&seed) :
                    kind==1 ? -2-33*bowling_uniform(&seed) : -8-22*bowling_uniform(&seed);
                t.release_going_back=kind==1;
                t.still=kind==3;
                if (t.still) t.release_s=1.3+.9*bowling_uniform(&seed);
                uint32_t noise_seed=seed;
                Outcome out=replay(&t,n&2 ? 60 : 100,noise_seed,false,false);
                throws[grip][kind]+=out.throws;
            }
        }
        printf("  %s randomized throws out of 500 per case: proper %u, going back %u, early return %u, still %u\n",
               grip ? "in-line" : "tip-forward",throws[grip][0],throws[grip][1],throws[grip][2],throws[grip][3]);
        CHECK(throws[grip][0]>=450 && !throws[grip][1] && !throws[grip][3],
              "valid release, outgoing backswing or stillness regression (grip %u)",grip);
    }
    /* Fixed-seed baselines are 15 and 66. Allow 5 and 14 extra throws per 500. */
    CHECK(throws[0][2]<=20,"tip-forward early-return misfires exceeded 20/500");
    CHECK(throws[1][2]<=80,"in-line early-return misfires exceeded 80/500");
}
int main(void) {
    TEST(exact_static_seeds);
    TEST(quasistatic_rotation_is_not_a_swing);
    TEST(named_bowling_scenarios);
    TEST(forward_power_and_clip_reconstruction);
    TEST(throw_type_power_and_cap);
    TEST(crooked_backswing_does_not_aim);
    TEST(low_radial_load_rejects_as_too_weak);
    TEST(release_during_turn_confirmation);
    TEST(sustained_noise_and_short_gaps);
    TEST(release_gap_and_overflow);
    TEST(retry_and_handed_controls);
    TEST(pending_release_expires_without_reports);
    TEST(randomized_releases);
    printf("%u throw-trace tests, %u failed checks\n",tests,failures);
    return failures ? 1 : 0;
}
