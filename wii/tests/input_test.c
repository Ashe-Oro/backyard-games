#include "input.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static unsigned tests;
#define TEST(fn) do { fn(); ++tests; printf("ok %u - %s\n",tests,#fn); } while (0)
static void near(double a, double b) { if (fabs(a-b)>=1e-9) fprintf(stderr,"near: %.12f != %.12f\n",a,b); assert(fabs(a-b)<1e-9); }
static SwingSample sample(uint64_t ms, double e, double pitch, double roll, bool b) {
    double p=pitch*SWING_PI/180, r=roll*SWING_PI/180;
    return (SwingSample){.ms=ms,.g={(1+e)*sin(r)*cos(p),(1+e)*sin(p),(1+e)*cos(r)*cos(p)},
        .b=b,.accel_valid=true};
}
static SwingProfile profile(void) { return swing_default_profile(); }
static void ready(Swing *s, double roll) {
    swing_reset(s,true);
    SwingProfile p=profile();
    for (unsigned t=0;t<=500;t+=10) {
        SwingSample v=sample(t,0,0,roll,false);
        assert(!swing_feed(s,&v,.04,&p).thrown);
    }
    assert(s->phase==SWING_READY);
}
static void arm(Swing *s, double roll) {
    ready(s,roll);
    SwingProfile p=profile();
    SwingSample v=sample(510,0,0,roll,true);
    assert(!swing_feed(s,&v,.04,&p).hold_still);
    assert(s->phase==SWING_GRIP);
}
static SwingResult replay(double e, unsigned duration, unsigned release_delay, bool auto_release, bool back, bool clip) {
    Swing s; arm(&s,0);
    SwingProfile p=profile();
    SwingResult shot={0};
    unsigned end=520+duration;
    for (unsigned t=520;t<=end+400;t+=10) {
        bool active=t<end;
        SwingSample v=sample(t,active?e:0,active?0:(back?-35:25),0,
                             auto_release || t<end+release_delay);
        v.clipped=clip && t==end-10;
        SwingResult r=swing_feed(&s,&v,-.12,&p);
        if (r.thrown) { assert(!shot.thrown); shot=r; }
    }
    return shot;
}
static void power_equation(void) {
    near(swing_speed(1.2),sqrt(.6*9.80665*1.2));
    near(swing_power(swing_speed(.3),swing_speed(1.2)),.5);
    near(swing_power(swing_speed(.048),swing_speed(1.2)),.2);
    near(swing_power(swing_speed(12),swing_speed(1.2)),1);
    near(swing_power(-1,3),0); near(swing_power(1,0),0);
    near(swing_power(NAN,3),0); near(swing_speed(INFINITY),0);
}
static void time_weighted_window(void) {
    SwingPoint p[]={{.ms=0,.e=0},{.ms=20,.e=2},{.ms=80,.e=2},{.ms=100,.e=0}};
    unsigned span;
    near(swing_window_mean(p,4,100,&span),1.75); assert(span==80);
    SwingPoint linear[]={{.ms=0,.e=0},{.ms=60,.e=3},{.ms=100,.e=5}};
    near(swing_window_mean(linear,3,100,&span),3); assert(span==80);
}
static void elapsed_span_not_sample_count(void) {
    Swing s; arm(&s,0); SwingProfile p=profile();
    for (unsigned t=511;t<=525;++t) {
        SwingSample v=sample(t,1.2,0,0,true);
        swing_feed(&s,&v,0,&p);
        assert(!s.lobe_valid);
    }
    assert(!s.lobe_valid);
}
static void speed_never_duration(void) {
    SwingResult short_fast=replay(1.2,180,0,false,false,false);
    SwingResult long_fast=replay(1.2,700,0,false,false,false);
    SwingResult long_slow=replay(.36,700,0,false,false,false);
    /* .36 is just over the qualifying threshold, yet remains much softer than a fast toss. */
    assert(short_fast.thrown && long_fast.thrown && long_slow.thrown);
    near(short_fast.power,long_fast.power);
    assert(long_slow.power<.6 && short_fast.power>.9);
}
static void release_latency_retains_peak(void) {
    SwingResult a=replay(1.0,180,0,false,false,false);
    SwingResult b=replay(1.0,180,30,false,false,false);
    SwingResult c=replay(1.0,180,60,false,false,false);
    assert(a.thrown && b.thrown && c.thrown);
    near(a.power,b.power); near(b.power,c.power);
    assert(a.release==SWING_B_UP && b.release==SWING_B_UP && c.release==SWING_B_UP);
}
static void auto_release_and_locks(void) {
    SwingResult r=replay(.8,180,0,true,false,false);
    assert(r.thrown && r.release==SWING_AUTO);
    near(r.aim,.04); near(r.spin,0); near(r.roll,0);
}
static void backswing_release_cancels(void) {
    assert(!replay(1.2,180,0,false,true,false).thrown);
    assert(!replay(1.2,180,0,true,true,false).thrown);
}
static void tap_b_and_wrist_rotation(void) {
    Swing s; arm(&s,0); SwingProfile p=profile();
    for (unsigned t=520;t<1200;t+=10) {
        SwingSample v=sample(t,0,0,(t%100)*2,false);
        assert(!swing_feed(&s,&v,0,&p).thrown);
    }
}
static void unstable_press_and_slow_rotation(void) {
    Swing s; ready(&s,0); SwingProfile p=profile();
    for (unsigned t=510;t<650;t+=10) {
        SwingSample v=sample(t,0,0,(double)(t-500)/10,false);
        swing_feed(&s,&v,0,&p);
    }
    SwingSample v=sample(650,0,0,15,true);
    assert(swing_feed(&s,&v,0,&p).hold_still); assert(s.phase==SWING_READY);
    ready(&s,0); v=sample(510,.5,0,0,true);
    assert(swing_feed(&s,&v,0,&p).hold_still);
}
static void gap_drop_and_bad_samples(void) {
    Swing s; SwingProfile p=profile();
    for (unsigned scenario=0;scenario<5;++scenario) {
        arm(&s,0); SwingSample v=sample(scenario==0?540:520,.8,0,0,true);
        if (scenario==1) v.dropped=1;
        if (scenario==2) v.accel_valid=false;
        if (scenario==3) v.g.x=NAN;
        if (scenario==4) v.ms=510;
        SwingResult r=swing_feed(&s,&v,0,&p);
        assert(r.cancelled && !r.thrown && s.phase==SWING_SETTLING);
    }
}
static void quiet_and_one_throw(void) {
    Swing s; swing_reset(&s,true); SwingProfile p=profile();
    for (unsigned t=0;t<=440;t+=10) {
        SwingSample v=sample(t,0,0,0,false); swing_feed(&s,&v,0,&p);
        assert(s.phase==SWING_SETTLING);
    }
    SwingSample v=sample(450,0,0,0,false); swing_feed(&s,&v,0,&p); assert(s.phase==SWING_READY);
    s.phase=SWING_SPENT;
    for (unsigned t=460;t<1000;t+=10) {
        v=sample(t,1,25,0,t%20!=0); assert(!swing_feed(&s,&v,0,&p).thrown);
        assert(s.phase==SWING_SPENT);
    }
}
static void clipping_never_validates(void) {
    Swing s; arm(&s,0); SwingProfile p=profile();
    for (unsigned t=520;t<1100;t+=10) {
        SwingSample v=sample(t,4,25,0,t<700); v.clipped=true;
        assert(!swing_feed(&s,&v,0,&p).thrown);
    }
    SwingResult r=replay(.8,180,0,false,false,true);
    assert(r.thrown && r.clipped); near(r.power,1);
    assert(!replay(.8,180,0,false,true,true).thrown);
}
static void roll_sign_deadzone_and_hand(void) {
    SwingProfile p=profile();
    near(swing_roll_spin(10,&p),0); near(swing_roll_spin(-10,&p),0);
    near(swing_roll_spin(45,&p),-1); near(swing_roll_spin(-45,&p),1);
    p.left_handed=true; p.spin_strength=.5;
    near(swing_roll_spin(45,&p),.5); near(swing_roll_spin(-90,&p),-.5);
}
static void last_lobe_not_backswing_peak(void) {
    Swing s; arm(&s,0); SwingProfile p=profile(); SwingResult shot={0};
    for (unsigned t=520;t<=1300;t+=10) {
        double e=t<700?1.2:(t>=850 && t<1030?.6:0);
        double pitch=t<700?0:(t<850?-35:(t<1030?0:25));
        SwingSample v=sample(t,e,pitch,0,t<1030);
        SwingResult r=swing_feed(&s,&v,0,&p); if (r.thrown) shot=r;
    }
    assert(shot.thrown); near(shot.power,sqrt(.6/1.2));
}
static void back_apex_then_slow_return_cannot_reuse_peak(void) {
    Swing s; arm(&s,0); SwingProfile p=profile();
    for (unsigned t=520;t<1200;t+=10) {
        SwingSample v=sample(t,t<700?1.2:0,t<700?0:(t<850?-35:0),0,t<870);
        assert(!swing_feed(&s,&v,0,&p).thrown);
    }
}
static void calibration_median_floor_clip_outlier(void) {
    SwingCalibration c={0}; SwingProfile p=profile();
    SwingCalibrationLimits lim={.target_power=.6,.full_speed_floor=1.5,.clip_ceiling=3};
    const double sets[][3]={{1.2,1.3,1.1},{.1,.11,.1},{2.5,2.4,2.6},{.1,1.2,1.3}};
    for (unsigned set=0;set<4;++set) {
        double before=p.full_speed;
        for (unsigned n=0;n<3;++n) {
            SwingResult r={.thrown=true,.speed=sets[set][n]};
            SwingCalibrationStatus status=swing_calibrate(&c,&r,lim,&p);
            assert(status==(n<2?SWING_CAL_MORE:(set==3?SWING_CAL_RETRY:SWING_CAL_DONE)));
        }
        if (set==0) near(p.full_speed,2);
        if (set==1) near(p.full_speed,1.5);
        if (set==2) near(p.full_speed,3);
        if (set==3) near(p.full_speed,before);
    }
    SwingResult bad={.thrown=true,.speed=1,.clipped=true};
    assert(swing_calibrate(&c,&bad,lim,&p)==SWING_CAL_RETRY);
    lim.target_power=0; assert(swing_calibrate(&c,&bad,lim,&p)==SWING_CAL_BAD_LIMITS);
}
static void trace_twist_uses_release_window_without_changing_curl(void) {
    SwingPoint points[]={
        {.ms=0,.g={99,0,0}}, {.ms=870,.g={-5,0,0}}, {.ms=900,.g={1,0,0}},
        {.ms=950,.g={4,0,0}}, {.ms=1000,.g={3,0,0}}, {.ms=1100,.g={2,0,0}},
        {.ms=1134,.g={99,0,0}}};
    near(swing_twist_relative(points,7,1000),10);
    SwingResult r=replay(.8,180,0,false,false,false);
    near(r.spin,0); assert(r.release_ms==700);
}

typedef struct { InputEvent events[256]; unsigned count; } Events;
static void sink(void *user, const InputEvent *event) {
    Events *events=user; assert(events->count<256); events->events[events->count++]=*event;
}
static unsigned occurrences(const Events *e, InputAction action) {
    unsigned n=0; for (unsigned k=0;k<e->count;++k) n+=e->events[k].action==action; return n;
}
static void input_sample(Input *i, unsigned ch, uint64_t t, uint32_t buttons, double e, double pitch) {
    InputSample p={.motion=sample(t,e,pitch,0,(buttons&INPUT_B)!=0),
        .buttons=buttons,.buttons_valid=true};
    input_feed(i,ch,&p);
}
static void input_ready(Input *i, Events *events, bool two) {
    *events=(Events){0}; input_init(i,sink,events);
    input_connection(i,0,true); if (two) input_connection(i,1,true);
    input_start_match(i);
    for (unsigned t=0;t<=500;t+=10) input_sample(i,0,t,0,0,0);
    assert(i->detector.phase==SWING_READY);
}
static void fine_aim_and_clamp(void) {
    Input i; Events e; input_ready(&i,&e,false);
    near(i.slots[0].aim,0);
    input_sample(&i,0,510,INPUT_PLUS,0,0); near(i.slots[0].aim,.01);
    input_sample(&i,0,520,INPUT_PLUS,0,0); near(i.slots[0].aim,.01);
    for (unsigned t=530;t<900;t+=10) input_sample(&i,0,t,t%20?0:INPUT_PLUS,0,0);
    near(i.slots[0].aim,.12);
}
static void buttons_mapping_and_easing(void) {
    Input i; Events e; input_ready(&i,&e,false);
    input_sample(&i,0,510,INPUT_UP|INPUT_A|INPUT_RIGHT|INPUT_ONE,0,0);
    assert(i.slots[0].height==2 && i.slots[0].sticky && i.help);
    near(i.slots[0].stance,.05); near(input_stance(&i,0,545),.025);
    near(input_stance(&i,0,580),.05); assert(i.detector.phase==SWING_SETTLING);
    input_sample(&i,0,520,0,0,0);
    input_sample(&i,0,530,INPUT_TWO,0,0);
    near(i.slots[0].stance,0); near(i.slots[0].aim,0);
    assert(occurrences(&e,INPUT_RESET_BAG)==1 && occurrences(&e,INPUT_HEIGHT)==1);
}
static void buttons_blocked_while_b_held(void) {
    Input i; Events e; input_ready(&i,&e,false);
    input_sample(&i,0,510,INPUT_B,0,0);
    assert(swing_armed(&i.detector));
    input_sample(&i,0,520,INPUT_B|INPUT_A|INPUT_TWO|INPUT_LEFT|INPUT_DOWN|INPUT_PLUS|INPUT_ONE,1,0);
    assert(i.slots[0].height==1 && !i.slots[0].sticky && i.help);
    near(i.slots[0].stance,0); near(i.slots[0].aim,0); near(i.detector.locked_aim,0);
    assert(!occurrences(&e,INPUT_RESET_BAG) && !occurrences(&e,INPUT_SURFACE));
    assert(swing_armed(&i.detector));
    input_sample(&i,0,530,INPUT_B|INPUT_HOME,1,0);
    assert(i.paused && !swing_armed(&i.detector) && occurrences(&e,INPUT_PAUSE)==1);
}
static void menu_confirm_back_navigation(void) {
    Input i; Events e; input_ready(&i,&e,false); input_set_context(&i,INPUT_MENU);
    input_sample(&i,0,510,INPUT_A|INPUT_UP|INPUT_MINUS,0,0);
    input_sample(&i,0,520,0,0,0);
    input_sample(&i,0,530,INPUT_B|INPUT_DOWN|INPUT_LEFT|INPUT_RIGHT|INPUT_PLUS,0,0);
    assert(occurrences(&e,INPUT_CONFIRM)==1 && occurrences(&e,INPUT_BACK)==1);
    assert(occurrences(&e,INPUT_NAV_UP)==1 && occurrences(&e,INPUT_NAV_DOWN)==1);
    assert(occurrences(&e,INPUT_ADJUST_MINUS)==1 && occurrences(&e,INPUT_ADJUST_PLUS)==1);
    assert(!i.slots[0].sticky);
    input_set_context(&i,INPUT_DISABLED);
    input_sample(&i,0,540,INPUT_A|INPUT_ONE,0,0);
    assert(i.help && !i.slots[0].sticky);
    input_sample(&i,0,550,INPUT_HOME,0,0);
    assert(i.paused && occurrences(&e,INPUT_PAUSE)==1);
}
static void slots_and_disconnect_reassignment(void) {
    Input i; Events e; input_ready(&i,&e,true);
    SwingProfile p={3,.4,true}; assert(input_set_profile(&i,1,p));
    input_next_turn(&i,1);
    input_sample(&i,0,510,INPUT_A,0,0); assert(!i.slots[1].sticky);
    input_sample(&i,1,510,INPUT_A,0,0); assert(i.slots[1].sticky);
    input_connection(&i,1,false);
    assert(i.paused && i.disconnect_pause && !input_resume(&i));
    assert(e.events[e.count-1].action==INPUT_CONNECT_REMOTE && e.events[e.count-1].channel==1);
    input_sample(&i,0,520,0,0,0);
    input_sample(&i,0,530,INPUT_A|INPUT_B,0,0);
    assert(i.channels[1]==0 && i.paused); /* A reassigns, held B still blocks resume. */
    input_sample(&i,0,540,0,0,0); assert(input_resume(&i));
    near(i.slots[1].profile.full_speed,3); assert(i.slots[1].profile.left_handed);
    assert(i.detector.phase==SWING_SETTLING);
}
static void pass_and_play_profile_is_slot_owned(void) {
    Input i; Events e; input_ready(&i,&e,false);
    assert(i.channels[1]==0);
    SwingProfile p={3,.2,true}; assert(input_set_profile(&i,1,p));
    input_next_turn(&i,1); input_connection(&i,1,true);
    assert(i.channels[1]==0); near(i.slots[1].profile.full_speed,3);
    near(i.slots[0].profile.full_speed,swing_speed(1.2));
    input_connection(&i,0,false); input_connection(&i,0,true);
    assert(!input_resume(&i));
    input_sample(&i,0,510,INPUT_B,0,0); assert(!input_resume(&i));
    input_sample(&i,0,520,0,0,0); assert(input_resume(&i));
}
static void input_throw_only_on_release_and_spent(void) {
    Input i; Events e; input_ready(&i,&e,false);
    input_sample(&i,0,510,INPUT_B,0,0);
    for (unsigned t=520;t<700;t+=10) {
        input_sample(&i,0,t,INPUT_B,1,0);
        assert(!occurrences(&e,INPUT_THROW));
    }
    for (unsigned t=700;t<1000;t+=10) input_sample(&i,0,t,0,0,25);
    assert(occurrences(&e,INPUT_THROW)==1 && i.detector.phase==SWING_SPENT);
    for (unsigned t=1000;t<2000;t+=10) input_sample(&i,0,t,t%20?INPUT_B:INPUT_A,1,25);
    assert(occurrences(&e,INPUT_THROW)==1);
}
static void calibration_route_does_not_throw_game_bags(void) {
    Input i; Events e; input_ready(&i,&e,false);
    assert(!input_begin_calibration(&i,(SwingCalibrationLimits){0,1,3}));
    assert(input_begin_calibration(&i,(SwingCalibrationLimits){.6,1.5,3}));
    for (unsigned t=510;t<=1010;t+=10) input_sample(&i,0,t,0,0,0);
    input_sample(&i,0,1020,INPUT_B,0,0);
    for (unsigned t=1030;t<1210;t+=10) input_sample(&i,0,t,INPUT_B,1,0);
    for (unsigned t=1210;t<1500;t+=10) input_sample(&i,0,t,0,0,25);
    assert(!occurrences(&e,INPUT_THROW) && occurrences(&e,INPUT_CAL_MORE)==1 && i.calibration.count==1);
    for (unsigned t=1500;t<=2000;t+=10) input_sample(&i,0,t,0,0,0);
    input_sample(&i,0,2010,INPUT_B,0,0);
    input_sample(&i,0,2020,0,0,0); /* tap B is not a calibration toss */
    assert(occurrences(&e,INPUT_CAL_RETRY)==1 && i.calibration.count==0);
    i.calibration.count=1; input_report_loss(&i);
    assert(occurrences(&e,INPUT_CAL_RETRY)==2 && i.calibration.count==0);
    input_cancel_calibration(&i); assert(!i.calibrating && i.calibration.count==0);
}

/* Equal-magnitude waveforms isolate direction/spin from speed power. */
static SwingResult directional_toss(double lateral, double release_roll, bool left, unsigned delay) {
    Swing s; arm(&s,0); SwingProfile p=profile(); p.left_handed=left;
    SwingResult shot={0};
    for (unsigned t=520;t<1100;t+=10) {
        bool moving=t<700;
        SwingSample v=sample(t,moving?.8:0,moving?0:25,moving?lateral:release_roll,t<700+delay);
        SwingResult result=swing_feed(&s,&v,0,&p);
        if (result.thrown) shot=result;
    }
    assert(shot.thrown); return shot;
}
static void swing_direction_and_spin_both_hands(void) {
    SwingResult straight=directional_toss(0,0,false,0);
    near(straight.aim,.04); /* arm() fine trim, independent of stance/hole */
    near(straight.spin,0);
    SwingResult right=directional_toss(12,0,false,0);
    SwingResult left=directional_toss(-12,0,false,0);
    assert(right.aim>straight.aim && left.aim<straight.aim);
    assert(right.spin<0 && left.spin>0);
    near(right.power,straight.power); near(left.power,straight.power);
    SwingResult mirrored=directional_toss(12,0,true,0);
    near(mirrored.aim-.04,-(right.aim-.04)); near(mirrored.spin,-right.spin);
    SwingResult twist=directional_toss(0,30,false,0);
    SwingResult twist_left=directional_toss(0,30,true,0);
    assert(twist.spin<0); near(twist_left.spin,-twist.spin);
    near(twist.aim,straight.aim); near(twist.power,straight.power);
    /* B-up freezes direction/spin before follow-through confirmation. */
    SwingResult delayed=directional_toss(12,0,false,30);
    near(delayed.aim,right.aim); near(delayed.power,right.power);
}
static void release_freezes_motion_controls(void) {
    Swing s; arm(&s,0); SwingProfile p=profile(); SwingResult shot={0};
    for (unsigned t=520;t<700;t+=10) {
        SwingSample v=sample(t,.8,0,12,true); swing_feed(&s,&v,0,&p);
    }
    SwingSample release=sample(700,0,25,30,false);
    swing_feed(&s,&release,0,&p);
    double aim=s.locked_aim, spin=s.locked_spin;
    assert(s.phase==SWING_PENDING);
    for (unsigned t=710;t<1000;t+=10) {
        SwingSample v=sample(t,0,25,-35,false);
        SwingResult result=swing_feed(&s,&v,-.12,&p);
        if (result.thrown) shot=result;
    }
    assert(shot.thrown); near(shot.aim,aim); near(shot.spin,spin);
}
static void backswing_direction_does_not_leak_to_forward(void) {
    Swing s; arm(&s,0); SwingProfile p=profile(); SwingResult shot={0};
    for (unsigned t=520;t<1350;t+=10) {
        bool back=t<700, forward=t>=850 && t<1030;
        SwingSample v=sample(t,back?1.2:(forward?.8:0),
                             back||forward?0:(t<850?-35:25),back?-20:0,t<1030);
        SwingResult result=swing_feed(&s,&v,0,&p);
        if (result.thrown) shot=result;
    }
    assert(shot.thrown); near(shot.aim,.04); near(shot.spin,0);
    near(shot.power,sqrt(.8/1.2));
}
static void straight_swing_from_shifted_stance(void) {
    Input i; Events e; input_ready(&i,&e,false);
    input_sample(&i,0,510,INPUT_RIGHT,0,0);
    for (unsigned t=520;t<=1020;t+=10) input_sample(&i,0,t,0,0,0);
    input_sample(&i,0,1030,INPUT_B,0,0);
    for (unsigned t=1040;t<1220;t+=10) input_sample(&i,0,t,INPUT_B,.8,0);
    for (unsigned t=1220;t<1500;t+=10) input_sample(&i,0,t,0,0,25);
    assert(occurrences(&e,INPUT_THROW)==1); near(i.slots[0].stance,.05);
    near(e.events[e.count-1].toss.aim,0); near(e.events[e.count-1].toss.spin,0);
}
static void setup_pause_hand_selection_and_calibration(void) {
    Input i; Events e; input_ready(&i,&e,false);
    assert(!input_focus_hand(&i,0));
    input_set_context(&i,INPUT_MENU); assert(input_focus_hand(&i,1));
    input_sample(&i,0,510,INPUT_LEFT,0,0);
    assert(i.slots[1].profile.left_handed && !i.slots[0].profile.left_handed);
    assert(e.events[e.count-1].slot==1 && e.events[e.count-1].action==INPUT_PROFILE_CHANGED);
    SwingProfile saved=i.slots[1].profile;
    SwingCalibration c={0};
    for (unsigned n=0;n<3;++n) {
        SwingResult toss={.thrown=true,.speed=1.2};
        swing_calibrate(&c,&toss,(SwingCalibrationLimits){.6,1,3},&saved);
    }
    assert(saved.left_handed); near(saved.full_speed,2);
    assert(input_set_profile(&i,1,saved));
    input_set_context(&i,INPUT_DISABLED); /* pause also works during flight */
    input_sample(&i,0,520,INPUT_HOME,0,0); assert(i.paused);
    assert(input_focus_hand(&i,1));
    input_sample(&i,0,530,INPUT_RIGHT,0,0);
    assert(!i.slots[1].profile.left_handed); near(i.slots[1].profile.full_speed,2);
    input_blur_hand(&i); assert(!i.hand_option);
    assert(strstr(input_help_text,"SWING LEFT/RIGHT TO AIM"));
}

/* Shoulder-pivot model: theta from hanging, remote Y tangent, Z radial.
   Specific force includes gravity, tangential and centripetal acceleration. */
static SwingSample pendulum(unsigned ms, double theta, double omega, double alpha, bool b) {
    return (SwingSample){.ms=ms, .g={0,
        SWING_RADIUS_M*alpha/SWING_GRAVITY+sin(theta),
        SWING_RADIUS_M*omega*omega/SWING_GRAVITY+cos(theta)},
        .b=b, .accel_valid=true};
}
static SwingResult physical_toss(double back_deg, double forward_deg, double forward_s,
                                 double release_deg, bool back_only, bool held, bool left) {
    Swing s; arm(&s,0); SwingProfile p=profile();
    p.full_speed=5; p.left_handed=left; /* Avoid saturation masking speed ordering. */
    SwingResult shot={0};
    double back=back_deg*SWING_PI/180, forward=forward_deg*SWING_PI/180;
    bool released=false;
    for (unsigned t=0;t<=2100;t+=10) {
        double seconds=t/1000.0, theta, omega=0, alpha=0;
        if (seconds<.5) {
            double phase=SWING_PI*seconds/.5;
            theta=-back*(1-cos(phase))/2;
            omega=-back*SWING_PI/.5*sin(phase)/2;
            alpha=-back*pow(SWING_PI/.5,2)*cos(phase)/2;
        } else if (back_only || seconds<.65) theta=-back;
        else if (seconds<.65+forward_s) {
            double phase=SWING_PI*(seconds-.65)/forward_s;
            theta=-back+(back+forward)*(1-cos(phase))/2;
            omega=(back+forward)*SWING_PI/forward_s*sin(phase)/2;
            alpha=(back+forward)*pow(SWING_PI/forward_s,2)*cos(phase)/2;
        } else theta=forward;
        if (!held && ((back_only && seconds>=.5) ||
            (!back_only && seconds>=.65 && theta>=release_deg*SWING_PI/180))) released=true;
        SwingSample v=pendulum(520+t,theta,omega,alpha,!released);
        SwingResult r=swing_feed(&s,&v,0,&p);
        if (back_only || seconds<.65) assert(!r.thrown);
        if (r.thrown) {
            assert(!shot.thrown);
            shot=r;
            if (!held) {
                assert(r.release==SWING_B_UP && r.release_ms==v.ms);
                /* Classified forward release must not wait for static follow-through. */
                assert(seconds<.65+forward_s);
            }
        }
    }
    return shot;
}
static void physical_backswing_never_throws(void) {
    for (unsigned hand=0;hand<2;++hand) for (unsigned angle=45;angle<=60;angle+=15) {
        assert(!physical_toss(angle,60,.5,0,true,true,hand).thrown);
        assert(!physical_toss(angle,60,.5,0,true,false,hand).thrown);
    }
}
static void physical_bottom_releases_and_speed(void) {
    for (unsigned hand=0;hand<2;++hand) {
        SwingResult held=physical_toss(45,60,.6,0,false,true,hand);
        assert(held.thrown && held.release==SWING_AUTO);
    }
    const double angles[]={-10,0,10,20};
    for (unsigned hand=0;hand<2;++hand) for (unsigned i=0;i<4;++i) {
        SwingResult slow=physical_toss(45,45,.6,angles[i],false,false,hand);
        SwingResult fast=physical_toss(45,45,.45,angles[i],false,false,hand);
        SwingResult wide=physical_toss(60,75,.6,angles[i],false,false,hand);
        SwingResult quick=physical_toss(60,75,.45,angles[i],false,false,hand);
        assert(slow.thrown && fast.thrown && wide.thrown && quick.thrown);
        assert(fast.power>slow.power && quick.power>wide.power);
    }
}
static void subgravity_is_not_calm(void) {
    Swing s; swing_reset(&s,true); SwingProfile p=profile();
    for (unsigned t=0;t<=600;t+=10) {
        SwingSample v=sample(t,-.22,0,0,false);
        swing_feed(&s,&v,0,&p);
        assert(s.phase==SWING_SETTLING);
    }
    ready(&s,0);
    for (unsigned t=510;t<=700;t+=10) {
        SwingSample v=sample(t,-.22,0,0,t==700);
        SwingResult r=swing_feed(&s,&v,0,&p);
        if (t==700) assert(r.hold_still && !swing_armed(&s));
    }
}

int main(void) {
    TEST(physical_backswing_never_throws); TEST(physical_bottom_releases_and_speed);
    TEST(subgravity_is_not_calm);
    TEST(release_freezes_motion_controls); TEST(backswing_direction_does_not_leak_to_forward);
    TEST(swing_direction_and_spin_both_hands); TEST(straight_swing_from_shifted_stance);
    TEST(setup_pause_hand_selection_and_calibration);
    TEST(power_equation); TEST(time_weighted_window); TEST(elapsed_span_not_sample_count);
    TEST(speed_never_duration); TEST(release_latency_retains_peak); TEST(auto_release_and_locks);
    TEST(backswing_release_cancels); TEST(tap_b_and_wrist_rotation); TEST(unstable_press_and_slow_rotation);
    TEST(gap_drop_and_bad_samples); TEST(quiet_and_one_throw); TEST(clipping_never_validates);
    TEST(roll_sign_deadzone_and_hand); TEST(last_lobe_not_backswing_peak);
    TEST(back_apex_then_slow_return_cannot_reuse_peak);
    TEST(calibration_median_floor_clip_outlier); TEST(fine_aim_and_clamp);
    TEST(trace_twist_uses_release_window_without_changing_curl);
    TEST(buttons_mapping_and_easing); TEST(buttons_blocked_while_b_held); TEST(menu_confirm_back_navigation);
    TEST(slots_and_disconnect_reassignment); TEST(pass_and_play_profile_is_slot_owned);
    TEST(input_throw_only_on_release_and_spent); TEST(calibration_route_does_not_throw_game_bags);
    printf("%u input tests passed\n",tests);
    return 0;
}
