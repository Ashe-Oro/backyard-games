#include "input.h"
#include <math.h>
#include <string.h>

/* 0.1.4: one line per control, split at " | " by the HUD. -/+ do nothing in play. */
const char input_help_text[] = "B HOLD, SWING BACK & FORWARD, LET GO | SWING SPEED = POWER | SWING LEFT/RIGHT TO AIM | TWIST TO CURL | D-PAD UP/DOWN KOBE-NORMAL-SLIDER | D-PAD LEFT/RIGHT MOVE | A SURFACE | 2 RESET | 1 HELP | HOME PAUSE";

static double bound(double v, double lo, double hi) { return fmax(lo, fmin(hi,v)); }
static void emit(Input *i, InputAction action, unsigned ch, double value, SwingResult toss) {
    InputEvent event = {action, i->active_slot, ch, value, toss};
    if (i->sink) i->sink(i->user, &event);
}
static void action(Input *i, InputAction a, unsigned ch, double value) {
    emit(i,a,ch,value,(SwingResult){0});
}
void input_init(Input *i, InputSink sink, void *user) {
    memset(i,0,sizeof(*i));
    i->sink=sink; i->user=user; i->context=INPUT_PLAY;
    for (unsigned s=0;s<2;++s) { i->slots[s].profile=swing_default_profile(); i->slots[s].height=1; }
    swing_reset(&i->detector,true);
}
void input_connection(Input *i, unsigned ch, bool connected) {
    if (ch>=2 || i->connected[ch]==connected) return;
    i->connected[ch]=connected;
    i->held[ch]=0;
    i->debug_holding[ch]=i->debug_latched[ch]=false;
    /* Unknown B state cannot satisfy the resume gate. */
    i->b_held[ch]=true;
    if (ch!=i->channels[i->active_slot]) return;
    input_report_loss(i);
    swing_reset(&i->detector,true);
    if (!connected && i->context!=INPUT_DISABLED) {
        i->paused=i->disconnect_pause=true;
        action(i,INPUT_CONNECT_REMOTE,ch,(double)(ch+1));
    }
}
void input_report_loss(Input *i) {
    if (i->calibrating && (swing_armed(&i->detector) || i->calibration.count)) {
        i->calibration.count=0;
        action(i,INPUT_CAL_RETRY,i->channels[i->active_slot],0);
    }
    i->debug_holding[i->channels[i->active_slot]]=false;
    if (i->detector.phase!=SWING_SPENT) swing_reset(&i->detector,true);
}
void input_start_match(Input *i) {
    i->channels[0]=0;
    i->channels[1]=i->connected[1] ? 1 : 0;
    i->paused=i->disconnect_pause=false;
    input_next_turn(i,0);
}
void input_next_turn(Input *i, unsigned slot) {
    if (slot>=2) return;
    if (slot!=i->active_slot) input_cancel_calibration(i);
    i->active_slot=slot;
    /* The turn is open: a B pressed during the bag flight or the turn change grips right away. */
    swing_reset(&i->detector,false);
    if (!i->connected[i->channels[slot]]) {
        i->paused=i->disconnect_pause=true;
        action(i,INPUT_CONNECT_REMOTE,i->channels[slot],(double)(i->channels[slot]+1));
    }
}
void input_set_context(Input *i, InputContext context) {
    i->context=context;
    i->hand_option=false;
    swing_reset(&i->detector,context!=INPUT_PLAY || i->paused);
}
bool input_resume(Input *i) {
    unsigned ch=i->channels[i->active_slot];
    if (!i->connected[ch] || i->b_held[ch]) return false;
    i->paused=i->disconnect_pause=false;
    i->hand_option=false;
    swing_reset(&i->detector,false); /* B is known up here: the next press grips */
    return true;
}
bool input_set_profile(Input *i, unsigned slot, SwingProfile p) {
    if (slot>=2 || !isfinite(p.full_speed) || p.full_speed<=0 ||
        !isfinite(p.spin_strength) || p.spin_strength<0 || p.spin_strength>1) return false;
    i->slots[slot].profile=p;
    if (slot==i->active_slot) swing_reset(&i->detector,true);
    return true;
}
bool input_begin_calibration(Input *i, SwingCalibrationLimits limits) {
    if (!isfinite(limits.target_power) || limits.target_power<=0 || limits.target_power>1 ||
        !isfinite(limits.full_speed_floor) || limits.full_speed_floor<=0 ||
        !isfinite(limits.clip_ceiling) || limits.clip_ceiling<limits.full_speed_floor) return false;
    i->calibration=(SwingCalibration){0};
    i->calibration_limits=limits;
    i->calibrating=true;
    swing_reset(&i->detector,true);
    return true;
}
void input_cancel_calibration(Input *i) {
    i->calibrating=false;
    i->calibration=(SwingCalibration){0};
    swing_reset(&i->detector,true);
}
double input_stance(const Input *i, unsigned slot, uint64_t now) {
    if (slot>=2) return 0;
    const InputSlot *s=&i->slots[slot];
    double t=now>=s->stance_ms ? bound((double)(now-s->stance_ms)/SWING_STEP_EASE_MS,0,1) : 0;
    return s->stance_from+(s->stance-s->stance_from)*t;
}
bool input_focus_hand(Input *i, unsigned slot) {
    if (slot>=2 || (i->context!=INPUT_MENU && !i->paused) || i->disconnect_pause) return false;
    i->hand_option=true; i->hand_slot=slot;
    return true;
}
void input_blur_hand(Input *i) { i->hand_option=false; }
static void debug_chord(Input *i, unsigned ch, const InputSample *p, uint32_t *pressed) {
    const uint32_t chord=INPUT_ONE|INPUT_TWO;
    if ((p->buttons&(chord|INPUT_B))!=chord) {
        i->debug_holding[ch]=i->debug_latched[ch]=false;
        return;
    }
    *pressed&=~chord;
    if (ch!=i->channels[i->active_slot]) return;
    uint64_t now=p->motion.ms;
    if (!i->debug_holding[ch] || now<i->debug_hold_ms[ch]) {
        i->debug_holding[ch]=true;
        i->debug_hold_ms[ch]=now;
    }
    if (!i->debug_latched[ch] && now-i->debug_hold_ms[ch]>=2000) {
        i->debug_visible=!i->debug_visible;
        i->debug_latched[ch]=true;
    }
}
static void track_stats(Input *i, const InputSample *p) {
    InputStats *st=&i->stats;
    uint64_t now=p->motion.ms;
    if (st->have_last && now>st->last_ms) {
        unsigned gap=(unsigned)fmin((double)(now-st->last_ms),99999);
        if (gap>st->window_gap_ms) st->window_gap_ms=gap;
    }
    if (!st->have_last || now<st->window_ms) { st->window_ms=now; st->window_count=0; }
    st->have_last=true;
    st->last_ms=now;
    ++st->window_count;
    uint64_t elapsed=now-st->window_ms;
    if (elapsed>=1000) {
        st->rate=(unsigned)lround(1000.0*(st->window_count-1)/(double)elapsed);
        st->window_count=1;
        st->window_ms=now;
        /* largest gap: published every other rate window (about 2 s) */
        if (++st->windows%2==0) { st->gap_ms=st->window_gap_ms; st->window_gap_ms=0; }
    }
}
unsigned input_largest_gap(const Input *i) {
    return i->stats.gap_ms>i->stats.window_gap_ms ? i->stats.gap_ms : i->stats.window_gap_ms;
}
void input_feed(Input *i, unsigned ch, const InputSample *p) {
    if (ch>=2 || !i->connected[ch]) return;
    if (ch==i->channels[i->active_slot]) track_stats(i,p);
    uint32_t pressed=0;
    if (p->buttons_valid) {
        pressed=p->buttons & ~i->held[ch];
        i->held[ch]=p->buttons;
        i->b_held[ch]=(p->buttons & INPUT_B)!=0;
        debug_chord(i,ch,p,&pressed);
    }
    /* The slot-2 disconnect prompt offers A: USE REMOTE 1 even while channel 1 is absent. */
    if (i->disconnect_pause && i->active_slot==1 && ch==0 && (pressed & INPUT_A)) {
        i->channels[1]=0;
        (void)input_resume(i);
        return;
    }
    /* Menus (strap, title, pause): A or 2 confirms, from any connected remote. No IR required. */
    if ((i->context==INPUT_MENU || i->paused) && !i->disconnect_pause) {
        if (i->hand_option && (pressed & (INPUT_LEFT|INPUT_RIGHT))) {
            unsigned slot_index=i->hand_slot;
            SwingProfile profile=i->slots[slot_index].profile;
            profile.left_handed=(pressed & INPUT_LEFT)!=0;
            (void)input_set_profile(i,slot_index,profile);
            input_cancel_calibration(i);
            InputEvent event={.action=INPUT_PROFILE_CHANGED,.slot=slot_index,.channel=ch,
                              .value=profile.left_handed};
            if (i->sink) i->sink(i->user,&event);
            pressed &= ~(INPUT_LEFT|INPUT_RIGHT);
        }
        if (pressed & (INPUT_A|INPUT_TWO)) action(i,INPUT_CONFIRM,ch,0);
        const uint32_t masks[]={INPUT_B,INPUT_UP,INPUT_DOWN,INPUT_LEFT,INPUT_RIGHT,INPUT_MINUS,INPUT_PLUS};
        const InputAction actions[]={INPUT_BACK,INPUT_NAV_UP,INPUT_NAV_DOWN,
                                    INPUT_NAV_LEFT,INPUT_NAV_RIGHT,INPUT_ADJUST_MINUS,INPUT_ADJUST_PLUS};
        for (unsigned n=0;n<sizeof(masks)/sizeof(masks[0]);++n)
            if (pressed & masks[n]) action(i,actions[n],ch,0);
        if (ch!=i->channels[i->active_slot] && !(pressed & INPUT_HOME) && !(pressed & INPUT_ONE)) return;
    }
    if (ch!=i->channels[i->active_slot]) return;
    InputSlot *slot=&i->slots[i->active_slot];
    if (pressed & INPUT_HOME) {
        i->paused=true;
        swing_reset(&i->detector,true);
        action(i,INPUT_PAUSE,ch,0);
        return;
    }
    if (pressed & INPUT_ONE) { i->help=!i->help; action(i,INPUT_HELP,ch,i->help); }
    if (i->context==INPUT_DISABLED && !i->paused) return;
    if (i->context==INPUT_MENU || i->paused) return;
    bool armed=swing_armed(&i->detector);
    bool b=i->b_held[ch];
    if (i->detector.phase==SWING_SPENT) return;
    if (!armed && !b) {
        /* -/+ are not gameplay controls (0.1.4): they neither aim nor reset the swing. */
        uint32_t changes=pressed & (INPUT_A|INPUT_UP|INPUT_DOWN|INPUT_LEFT|INPUT_RIGHT|INPUT_TWO);
        if (changes) {
            if (changes & INPUT_A) { slot->sticky=!slot->sticky; action(i,INPUT_SURFACE,ch,slot->sticky); }
            if (changes & (INPUT_UP|INPUT_DOWN)) {
                slot->height=(int)bound(slot->height+((changes&INPUT_UP)?1:0)-((changes&INPUT_DOWN)?1:0),0,2);
                action(i,INPUT_HEIGHT,ch,slot->height);
            }
            if (changes & (INPUT_LEFT|INPUT_RIGHT)) {
                slot->stance_from=input_stance(i,i->active_slot,p->motion.ms);
                slot->stance=bound(slot->stance+((changes&INPUT_RIGHT)?SWING_STEP:0)-
                                   ((changes&INPUT_LEFT)?SWING_STEP:0),-SWING_STEP_CLAMP,SWING_STEP_CLAMP);
                slot->stance_ms=p->motion.ms;
                action(i,INPUT_STEP,ch,slot->stance);
            }
            if (changes & INPUT_TWO) {
                slot->stance=slot->stance_from=0;
                action(i,INPUT_RESET_BAG,ch,0);
            }
            swing_reset(&i->detector,true);
            return;
        }
    }
    /* A report without valid buttons cannot move B; skip it rather than guess. */
    if (!p->buttons_valid) return;
    SwingSample motion=p->motion;
    motion.b=b;
    motion.manual_release=true;
    SwingResult r=swing_feed(&i->detector,&motion,&slot->profile);
    if (r.thrown) {
        i->stats.last_toss=r;
        i->stats.have_toss=true;
        if (i->calibrating) {
            SwingCalibrationStatus status=swing_calibrate(&i->calibration,&r,i->calibration_limits,&slot->profile);
            InputAction a=status==SWING_CAL_DONE ? INPUT_CAL_DONE : status==SWING_CAL_MORE ? INPUT_CAL_MORE : INPUT_CAL_RETRY;
            emit(i,a,ch,i->calibration.count,r);
            if (status==SWING_CAL_DONE) i->calibrating=false;
            swing_reset(&i->detector,true);
        } else emit(i,INPUT_THROW,ch,r.power,r);
    }
}
