#include "input.h"
#include "bowling_trace.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static unsigned tests;
#define TEST(fn) do { fn(); ++tests; printf("ok %u - %s\n",tests,#fn); } while (0)
static void near(double a, double b) { if (fabs(a-b)>=1e-9) fprintf(stderr,"near: %.12f != %.12f\n",a,b); assert(fabs(a-b)<1e-9); }
static SwingSample sample(uint64_t ms, double e, double pitch, double roll, bool b) {
    double p=pitch*SWING_PI/180, r=roll*SWING_PI/180;
    return (SwingSample){.ms=ms,.g={(1+e)*sin(r)*cos(p),-(1+e)*sin(p),(1+e)*cos(r)*cos(p)},
        .b=b,.accel_valid=true};
}
static SwingProfile profile(void) { return swing_default_profile(); }
static void ready(Swing *s, double roll) {
    swing_reset(s,true);
    SwingProfile p=profile();
    for (unsigned t=0;t<=500;t+=10) {
        SwingSample v=sample(t,0,0,roll,false);
        assert(!swing_feed(s,&v,&p,1).thrown);
    }
    assert(s->phase==SWING_READY);
}
static void arm(Swing *s, double roll) {
    ready(s,roll);
    SwingProfile p=profile();
    SwingSample v=sample(510,0,0,roll,true);
    assert(!swing_feed(s,&v,&p,1).thrown);
    assert(s->phase==SWING_GRIP);
}
static void power_equation(void) {
    double full=profile().full_speed;
    near(full,SWING_DEFAULT_FULL_SPEED);
    near(swing_power(.1*full,full),.0465208198);
    near(swing_power(.2*full,full),.1444116608);
    near(swing_power(.5*full,full),.4601460338);
    near(swing_power(full,full),.7280505916);
    near(swing_power(swing_speed(4),full),.76);
    near(swing_power(swing_speed(12),full),.76);
    near(swing_power(-1,3),0); near(swing_power(1,0),0);
    near(swing_power(NAN,3),0); near(swing_speed(INFINITY),0);
    const double scales[]={1.7637,1.4216,1.1618};
    const double previous_scales[]={1.30,1.20,1.0/.90};
    const double ratio[]={27.0/18,27.0/21,27.0/25};
    const double exponent[]={1.3291,1.4833,1.7257};
    for (int h=0;h<3;++h) {
        near(swing_type_scale(h),scales[h]);
        double effective=full*swing_type_scale(h);
        assert(fabs(pow(scales[h]/previous_scales[h],exponent[h])-ratio[h]) < .0001);
        near(swing_power(swing_speed(12),effective),.76);
        const double powers[]={.03,.25,.50,.70,.76};
        for (unsigned p=0;p<sizeof powers/sizeof powers[0];++p) {
            double peak=swing_speed(.8);
            double reference=swing_full_speed_for_power(peak,powers[p],h);
            near(swing_power(peak,reference*scales[h]),powers[p]);
        }
    }
}
/* 0.1.2 answered these presses with HOLD STILL and no grip. */
static void moving_press_grips_immediately(void) {
    Swing s; ready(&s,0); SwingProfile p=profile();
    for (unsigned t=510;t<650;t+=10) {
        SwingSample v=sample(t,0,0,(double)(t-500)/10,false);
        swing_feed(&s,&v,&p,1);
    }
    SwingSample v=sample(650,0,0,15,true);
    swing_feed(&s,&v,&p,1); assert(s.phase==SWING_GRIP);
    ready(&s,0); v=sample(510,.5,0,0,true);
    swing_feed(&s,&v,&p,1); assert(s.phase==SWING_GRIP);
    /* No history at all (first report of the turn) still grips, using that report. */
    swing_reset(&s,false); v=sample(0,1.4,70,0,true);
    swing_feed(&s,&v,&p,1); assert(s.phase==SWING_GRIP);
}
static void gaps_drops_and_bad_samples_keep_grip(void) {
    Swing s; SwingProfile p=profile();
    for (unsigned scenario=0;scenario<6;++scenario) {
        arm(&s,0); SwingSample v=sample(scenario==0?540:520,.8,0,0,true);
        if (scenario==1) v.dropped=1;
        if (scenario==2) v.accel_valid=false;
        if (scenario==3) v.g.x=NAN;
        if (scenario==4) v.ms=510;
        if (scenario==5) v.ms=610; /* 100 ms gap: bridged */
        SwingResult r=swing_feed(&s,&v,&p,1);
        assert(!r.thrown && swing_armed(&s));
    }
    assert(s.bridged==1);
}
static void ready_at_once_and_one_throw(void) {
    Swing s; swing_reset(&s,true); SwingProfile p=profile();
    assert(s.phase==SWING_READY);
    SwingSample v=sample(0,0,0,0,true); swing_feed(&s,&v,&p,1);
    assert(s.phase==SWING_READY); /* require_b_up: a held B must be let go first */
    v=sample(10,0,0,0,false); swing_feed(&s,&v,&p,1);
    v=sample(20,0,0,0,true); swing_feed(&s,&v,&p,1); assert(s.phase==SWING_GRIP);
    s.phase=SWING_SPENT;
    for (unsigned t=460;t<1000;t+=10) {
        v=sample(t,1,25,0,t%20!=0); assert(!swing_feed(&s,&v,&p,1).thrown);
        assert(s.phase==SWING_SPENT);
    }
}
static void roll_sign_deadzone_and_hand(void) {
    SwingProfile p=profile();
    near(swing_roll_spin(10,&p),0); near(swing_roll_spin(-10,&p),0);
    near(swing_roll_spin(45,&p),-1); near(swing_roll_spin(-45,&p),1);
    p.left_handed=true; p.spin_strength=.5;
    near(swing_roll_spin(45,&p),.5); near(swing_roll_spin(-90,&p),-.5);
}
static void calibration_median_floor_clip_outlier(void) {
    SwingCalibration c={0}; SwingProfile p=profile();
    SwingCalibrationLimits lim={.target_power=.6,.full_speed_floor=1.25,.clip_ceiling=3};
    const double sets[][3]={{2.1,2.2,2},{.1,.11,.1},{4.8,4.7,4.9},{.1,1.2,1.3}};
    for (unsigned set=0;set<4;++set) {
        double before=p.full_speed;
        for (unsigned n=0;n<3;++n) {
            SwingResult r={.thrown=true,.speed=sets[set][n]};
            SwingCalibrationStatus status=swing_calibrate(&c,&r,lim,&p);
            assert(status==(n<2?SWING_CAL_MORE:(set==3?SWING_CAL_RETRY:SWING_CAL_DONE)));
        }
        if (set==0) {
            double reference=swing_type_scale(1);
            near(p.full_speed,swing_full_speed_for_power(2.1,lim.target_power,1));
            near(swing_power(2.1,p.full_speed*reference),lim.target_power);
        }
        if (set==1) near(p.full_speed,1.25);
        if (set==2) near(p.full_speed,3);
        if (set==3) near(p.full_speed,before);
    }
    SwingResult bad={.thrown=true,.speed=1,.clipped=true};
    assert(swing_calibrate(&c,&bad,lim,&p)==SWING_CAL_RETRY);
    lim.target_power=0; assert(swing_calibrate(&c,&bad,lim,&p)==SWING_CAL_BAD_LIMITS);
    lim.target_power=.8; assert(swing_calibrate(&c,&bad,lim,&p)==SWING_CAL_BAD_LIMITS);
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
/* Returns throws seen before the physical B-up edge. */
static unsigned feed_toss(Input *i, Events *events, const BowlingTrace *trace,
                          unsigned base, uint32_t extra) {
    unsigned early=0;
    double release=bowling_release(trace);
    for (unsigned tick=0;tick<3000;tick+=10) {
        double s=tick/1000.0;
        bool b=bowling_button(trace,s);
        uint32_t buttons=(b ? INPUT_B : 0) | ((tick/10)%2 ? extra : 0);
        InputSample report={.motion={.ms=base+tick,.g=bowling_force(trace,s),.accel_valid=true},
            .buttons=buttons,.buttons_valid=true};
        input_feed(i,0,&report);
        if (s<release) early=occurrences(events,INPUT_THROW);
    }
    return early;
}

static SwingResult input_swing(uint32_t extra, bool extra_up) {
    Input i; Events e; input_ready(&i,&e,false);
    for (unsigned t=510;t<600;t+=10)
        input_sample(&i,0,t,extra_up && (t/10)%2 ? extra : 0,0,0);
    BowlingTrace trace=bowling_trace();
    assert(!feed_toss(&i,&e,&trace,600,extra));
    assert(occurrences(&e,INPUT_THROW)==1);
    assert(!occurrences(&e,INPUT_ADJUST_MINUS) && !occurrences(&e,INPUT_ADJUST_PLUS));
    for (unsigned n=0;n<e.count;++n) if (e.events[n].action==INPUT_THROW) return e.events[n].toss;
    return (SwingResult){0};
}
/* 0.1.4: in gameplay -/+ change nothing about the throw (no fine-aim nudge). */
static void plus_minus_change_nothing_in_play(void) {
    SwingResult plain=input_swing(0,false);
    const uint32_t keys[]={INPUT_PLUS,INPUT_MINUS,INPUT_PLUS|INPUT_MINUS};
    for (unsigned k=0;k<3;++k) for (int up=0;up<2;++up) {
        SwingResult r=input_swing(keys[k],up);
        near(r.power,plain.power); near(r.aim,plain.aim); near(r.spin,plain.spin);
        assert(r.release==plain.release);
    }
    /* Pressing + ten times before the grip leaves the grip ready (no reset, no event). */
    Input i; Events e; input_ready(&i,&e,false);
    for (unsigned t=510;t<710;t+=10) input_sample(&i,0,t,(t/10)%2?INPUT_PLUS:INPUT_MINUS,0,0);
    assert(e.count==0 && i.detector.phase==SWING_READY && !i.detector.require_b_up);
    input_sample(&i,0,710,INPUT_B|INPUT_PLUS,0,0);
    assert(i.detector.phase==SWING_GRIP);
}
static void buttons_mapping_and_easing(void) {
    Input i; Events e; input_ready(&i,&e,false);
    input_sample(&i,0,510,INPUT_UP|INPUT_A|INPUT_RIGHT|INPUT_ONE,0,0);
    assert(i.slots[0].height==2 && i.slots[0].sticky && i.help);
    near(i.slots[0].stance,.05); near(input_stance(&i,0,545),.025);
    near(input_stance(&i,0,580),.05); assert(i.detector.phase==SWING_READY);
    input_sample(&i,0,520,0,0,0);
    input_sample(&i,0,530,INPUT_TWO,0,0);
    near(i.slots[0].stance,0);
    assert(occurrences(&e,INPUT_RESET_BAG)==1 && occurrences(&e,INPUT_HEIGHT)==1);
}
static void buttons_blocked_while_b_held(void) {
    Input i; Events e; input_ready(&i,&e,false);
    input_sample(&i,0,510,INPUT_B,0,0);
    assert(swing_armed(&i.detector));
    input_sample(&i,0,520,INPUT_B|INPUT_A|INPUT_TWO|INPUT_LEFT|INPUT_DOWN|INPUT_PLUS|INPUT_ONE,1,0);
    assert(i.slots[0].height==1 && !i.slots[0].sticky && i.help);
    near(i.slots[0].stance,0);
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
    assert(i.detector.phase==SWING_READY);
}
static void pass_and_play_profile_is_slot_owned(void) {
    Input i; Events e; input_ready(&i,&e,false);
    assert(i.channels[1]==0);
    SwingProfile p={3,.2,true}; assert(input_set_profile(&i,1,p));
    input_next_turn(&i,1); input_connection(&i,1,true);
    assert(i.channels[1]==0); near(i.slots[1].profile.full_speed,3);
    near(i.slots[0].profile.full_speed,SWING_DEFAULT_FULL_SPEED);
    input_connection(&i,0,false); input_connection(&i,0,true);
    assert(!input_resume(&i));
    input_sample(&i,0,510,INPUT_B,0,0); assert(!input_resume(&i));
    input_sample(&i,0,520,0,0,0); assert(input_resume(&i));
}
static void input_throw_only_on_release_and_spent(void) {
    Input i; Events e; input_ready(&i,&e,false);
    BowlingTrace trace=bowling_trace();
    assert(!feed_toss(&i,&e,&trace,510,0));
    assert(occurrences(&e,INPUT_THROW)==1 && i.detector.phase==SWING_SPENT);
    for (unsigned t=3520;t<3620;t+=10) input_sample(&i,0,t,t%20 ? INPUT_B : INPUT_A,1,25);
    assert(occurrences(&e,INPUT_THROW)==1);
}
static void calibration_route_does_not_throw_game_bags(void) {
    Input i; Events e; input_ready(&i,&e,false);
    assert(!input_begin_calibration(&i,(SwingCalibrationLimits){0,1,3}));
    assert(input_begin_calibration(&i,(SwingCalibrationLimits){.6,1.5,3}));
    BowlingTrace trace=bowling_trace();
    trace.forward_s=.65;
    assert(!feed_toss(&i,&e,&trace,510,0));
    assert(!occurrences(&e,INPUT_THROW) && occurrences(&e,INPUT_CAL_MORE)==1 && i.calibration.count==1);
    input_sample(&i,0,3520,INPUT_B,0,0);
    input_sample(&i,0,3530,0,0,0); /* tap B is not a calibration toss */
    assert(!occurrences(&e,INPUT_CAL_RETRY) && i.calibration.count==1);
    i.calibration.count=1; input_report_loss(&i);
    assert(occurrences(&e,INPUT_CAL_RETRY)==1 && i.calibration.count==0);
    input_cancel_calibration(&i); assert(!i.calibrating && i.calibration.count==0);
}

static void straight_swing_from_shifted_stance(void) {
    Input i; Events e; input_ready(&i,&e,false);
    input_sample(&i,0,510,INPUT_RIGHT,0,0);
    BowlingTrace trace=bowling_trace();
    assert(!feed_toss(&i,&e,&trace,520,0));
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
    assert(saved.left_handed); near(swing_power(1.2,saved.full_speed*swing_type_scale(1)),.6);
    assert(input_set_profile(&i,1,saved));
    input_set_context(&i,INPUT_DISABLED); /* pause also works during flight */
    input_sample(&i,0,520,INPUT_HOME,0,0); assert(i.paused);
    assert(input_focus_hand(&i,1));
    input_sample(&i,0,530,INPUT_RIGHT,0,0);
    assert(!i.slots[1].profile.left_handed); near(i.slots[1].profile.full_speed,saved.full_speed);
    input_blur_hand(&i); assert(!i.hand_option);
    assert(strstr(input_help_text,"SWING LEFT/RIGHT TO AIM"));
    /* 0.1.4 help matches the controls: speed = power, D-pad up/down = arc, no -/+ fine aim. */
    const char *want[]={"B HOLD, SWING BACK & FORWARD, LET GO","SWING SPEED = POWER","TWIST TO CURL",
        "D-PAD UP/DOWN KOBE-NORMAL-SLIDER","D-PAD LEFT/RIGHT MOVE","A SURFACE","2 RESET","1 HELP","HOME PAUSE"};
    for (unsigned n=0;n<sizeof want/sizeof want[0];++n) assert(strstr(input_help_text,want[n]));
    assert(!strstr(input_help_text,"-/+") && !strstr(input_help_text,"FINE AIM") && !strstr(input_help_text,"POINT"));
}

static void subgravity_press_still_grips(void) {
    Swing s; ready(&s,0); SwingProfile p=profile();
    for (unsigned t=510;t<=700;t+=10) {
        SwingSample v=sample(t,-.22,0,0,t==700);
        swing_feed(&s,&v,&p,1);
        if (t==700) assert(swing_armed(&s));
    }
    SwingSample v=sample(710,-.22,0,0,false);
    SwingResult r=swing_feed(&s,&v,&p,1);
    assert(!r.thrown && s.phase==SWING_READY);
}


static void menu_a_or_two_from_any_remote(void) {
    Input i; Events e;
    input_init(&i,sink,&e); input_connection(&i,0,true); input_connection(&i,1,true);
    input_set_context(&i,INPUT_MENU);
    /* Channel 1 is not the active slot (0). A and 2 both confirm. */
    e=(Events){0};
    input_sample(&i,1,10,INPUT_A,0,0);
    assert(occurrences(&e,INPUT_CONFIRM)==1 && e.events[0].channel==1);
    e=(Events){0};
    input_sample(&i,1,20,0,0,0);
    input_sample(&i,0,30,INPUT_TWO,0,0);
    assert(occurrences(&e,INPUT_CONFIRM)==1);
    /* Play mode still ignores the inactive channel. */
    input_set_context(&i,INPUT_PLAY); input_start_match(&i);
    e=(Events){0};
    input_sample(&i,1,40,INPUT_A,0,0);
    assert(!occurrences(&e,INPUT_CONFIRM) && !i.slots[0].sticky);
    input_sample(&i,0,50,INPUT_A,0,0);
    assert(i.slots[0].sticky);
}
static void bowling_waits_for_b_release_after_follow_through(void) {
    Input i; Events e; input_ready(&i,&e,false);
    BowlingTrace trace=bowling_trace();
    trace.release_s=1.95; /* After motion stops, within the forward release window. */
    assert(!feed_toss(&i,&e,&trace,510,0));
    assert(occurrences(&e,INPUT_THROW)==1 && e.events[e.count-1].toss.release==SWING_B_UP);
}
static void debug_chord_requires_two_seconds_and_release(void) {
    Input i; Events e; input_ready(&i,&e,true);
    assert(!i.debug_visible);
    uint32_t chord=INPUT_ONE|INPUT_TWO;
    for (unsigned t=510;t<=2510;t+=10) input_sample(&i,1,t,chord,0,0);
    assert(!i.debug_visible);
    for (unsigned t=510;t<2510;t+=10) input_sample(&i,0,t,chord,0,0);
    assert(!i.debug_visible);
    input_sample(&i,0,2510,chord,0,0);
    assert(i.debug_visible && !e.count && !i.help);
    input_next_turn(&i,0);
    for (unsigned t=2520;t<=5000;t+=10) input_sample(&i,0,t,chord,0,0);
    assert(i.debug_visible && !e.count);
    input_sample(&i,0,5010,0,0,0);
    for (unsigned t=5020;t<=7020;t+=10) input_sample(&i,0,t,chord,0,0);
    assert(!i.debug_visible && !e.count);
}
int main(void) {
    TEST(subgravity_press_still_grips); TEST(straight_swing_from_shifted_stance);
    TEST(setup_pause_hand_selection_and_calibration); TEST(power_equation);
    TEST(moving_press_grips_immediately); TEST(gaps_drops_and_bad_samples_keep_grip);
    TEST(ready_at_once_and_one_throw); TEST(roll_sign_deadzone_and_hand);
    TEST(calibration_median_floor_clip_outlier); TEST(plus_minus_change_nothing_in_play);
    TEST(buttons_mapping_and_easing); TEST(buttons_blocked_while_b_held); TEST(menu_confirm_back_navigation);
    TEST(slots_and_disconnect_reassignment); TEST(pass_and_play_profile_is_slot_owned);
    TEST(input_throw_only_on_release_and_spent); TEST(calibration_route_does_not_throw_game_bags);
    TEST(menu_a_or_two_from_any_remote); TEST(bowling_waits_for_b_release_after_follow_through);
    TEST(debug_chord_requires_two_seconds_and_release);
    printf("%u input tests passed\n",tests);
    return 0;
}
