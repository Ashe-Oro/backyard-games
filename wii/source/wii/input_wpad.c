#include "input_wpad.h"
#include <math.h>
#include <string.h>

_Static_assert(INPUT_A==WPAD_BUTTON_A && INPUT_B==WPAD_BUTTON_B &&
               INPUT_ONE==WPAD_BUTTON_1 && INPUT_TWO==WPAD_BUTTON_2 &&
               INPUT_HOME==WPAD_BUTTON_HOME && INPUT_MINUS==WPAD_BUTTON_MINUS &&
               INPUT_PLUS==WPAD_BUTTON_PLUS && INPUT_LEFT==WPAD_BUTTON_LEFT &&
               INPUT_RIGHT==WPAD_BUTTON_RIGHT && INPUT_UP==WPAD_BUTTON_UP &&
               INPUT_DOWN==WPAD_BUTTON_DOWN, "WPAD button map changed");

/* libogc's synchronous ReadPending callback has no user context. */
static InputWpad *polling_wpad;

static void collect_report(int32_t ch, const WPADData *report) {
    (void)ch;
    InputWpad *w=polling_wpad;
    if (w->batch_count<INPUT_WPAD_EVENTS) w->batch[w->batch_count++]=*report;
}

int input_wpad_init(InputWpad *w, Input *input, InputWpadTraceSink trace, void *user) {
    memset(w,0,sizeof(*w));
    w->input=input; w->trace=trace; w->trace_user=user;
    for (unsigned ch=0;ch<2;++ch)
        for (unsigned axis=0;axis<3;++axis) w->raw_high[ch][axis]=1023;
    return WPAD_Init();
}
bool input_wpad_set_rails(InputWpad *w, unsigned ch, const uint16_t lo[3], const uint16_t hi[3]) {
    if (ch>=2) return false;
    for (unsigned a=0;a<3;++a) if (hi[a]<=lo[a]+4 || hi[a]>1023) return false;
    memcpy(w->raw_low[ch],lo,sizeof(w->raw_low[ch]));
    memcpy(w->raw_high[ch],hi,sizeof(w->raw_high[ch]));
    if (ch==w->input->channels[w->input->active_slot]) swing_reset(&w->input->detector,true);
    return true;
}
static int register_channel(InputWpad *w, unsigned ch) {
    /* ACC mode requests a continuous stream of buttons and accelerometer reports. */
    int rc=WPAD_SetDataFormat((int)ch,WPAD_FMT_BTNS_ACC);
    if (rc<0) return rc;
    rc=WPAD_SetIdleThresholds((int)ch,WPAD_THRESH_ANY,WPAD_THRESH_IGNORE,WPAD_THRESH_ANY,
                            WPAD_THRESH_IGNORE,WPAD_THRESH_IGNORE,WPAD_THRESH_IGNORE);
    if (rc<0) return rc;
    return WPAD_SetEventBufs((int)ch,w->events[ch],INPUT_WPAD_EVENTS);
}
static void lost_channel(InputWpad *w, unsigned ch) {
    w->registered[ch]=w->have_sample[ch]=w->have_event_path[ch]=false;
    w->latest[ch]=(InputSample){0};
    input_connection(w->input,ch,false);
}
static void feed_report(InputWpad *w, unsigned ch, WPADData *d, unsigned n, unsigned drops,
                        uint64_t sample_ms, bool reject, uint64_t now) {
    Input *i=w->input;
    InputSample sample={0};
    sample.motion.ms=sample_ms;
    sample.motion.g=(SwingVector){d->gforce.x,d->gforce.y,d->gforce.z};
    bool clean=d->err==WPAD_ERR_NONE;
    /* Honor an explicit data_present mask. If the mask is empty (some ScanPads
     * snapshots), accept accel when g-force or raw axes are actually filled. */
    if (d->data_present)
        sample.motion.accel_valid=clean && (d->data_present&WPAD_DATA_ACCEL)!=0;
    else
        sample.motion.accel_valid=clean && (d->gforce.x!=0 || d->gforce.y!=0 ||
            d->gforce.z!=0 || d->accel.x || d->accel.y || d->accel.z);
    sample.buttons_valid=clean;
    sample.buttons=d->btns_h;
    sample.motion.b=(sample.buttons&INPUT_B)!=0;
    sample.motion.dropped=n==0 ? drops : 0;
    const uint16_t raw[]={d->accel.x,d->accel.y,d->accel.z};
    for (unsigned a=0;a<3;++a)
        if (raw[a]<=w->raw_low[ch][a]+2 || raw[a]>=w->raw_high[ch][a]-2)
            sample.motion.clipped=true;
    w->latest[ch]=sample;
    if (!reject) input_feed(i,ch,&sample);
    if (w->trace) {
        InputWpadTrace row={ch,n,i->active_slot,now,sample.motion.ms,
            sample.motion.dropped,true,reject,d,&sample,&i->detector};
        w->trace(w->trace_user,&row);
    }
}
void input_wpad_poll(InputWpad *w, uint64_t now) {
    Input *i=w->input;
    polling_wpad=w;
    for (unsigned ch=0;ch<2;++ch) {
        int rc=WPAD_Probe((int)ch,NULL);
        w->status[ch]=rc;
        if (rc<0) { lost_channel(w,ch); continue; }
        if (!w->registered[ch]) {
            rc=register_channel(w,ch);
            w->status[ch]=rc;
            if (rc<0) { lost_channel(w,ch); continue; }
            w->registered[ch]=true;
            input_connection(i,ch,true);
        }

        /* ScanPads is ReadPending with a null callback. It already drains the
         * queue and coalesces button edges. Read each report before coalescing
         * so the B state and acceleration stay paired. */
        w->batch_count=0;
        rc=WPAD_ReadPending((int)ch,collect_report);
        if (rc<0) { w->status[ch]=rc; lost_channel(w,ch); continue; }
        unsigned count=w->batch_count;
        if (count) w->have_event_path[ch]=true;
        else if (!w->have_event_path[ch]) {
            const WPADData *cur=WPAD_Data((int)ch);
            if (cur && cur->err==WPAD_ERR_NONE && cur->data_present) {
                w->batch[0]=*cur;
                count=1;
            }
        }

        /* WPAD reports arrive without timestamps. Partition the observed
         * interval across the batch instead of subtracting a fixed 10 ms per
         * report from the end of a ~17 ms video frame. */
        int drops=WPAD_DroppedEvents((int)ch);
        if (drops<0) { w->status[ch]=drops; lost_channel(w,ch); continue; }
        w->dropped[ch]+=(unsigned)drops;
        w->reports[ch]+=count;
        uint64_t nominal=(uint64_t)count*INPUT_WPAD_REPORT_MS;
        uint64_t start=now>=nominal ? now-nominal : 0;
        if (w->have_sample[ch]) start=w->last_sample_ms[ch];
        uint64_t elapsed=now>start ? now-start : 0;
        bool reject=drops>0 || count==INPUT_WPAD_EVENTS ||
            (count && (elapsed<count || elapsed>(uint64_t)count*SWING_GAP_MS));
        bool gap=!count && w->have_sample[ch] && elapsed>SWING_GAP_MS;
        if (reject) ++w->rejected_batches[ch];
        if ((reject || gap) && ch==i->channels[i->active_slot]) input_report_loss(i);
        if (gap) {
            w->latest[ch].motion.accel_valid=false;
            w->latest[ch].buttons_valid=false;
        }
        for (unsigned n=0;n<count;++n)
            feed_report(w,ch,&w->batch[n],n,(unsigned)drops,
                        start+(n+1)*elapsed/count,reject,now);
        if (count) {
            w->have_sample[ch]=true;
            w->last_sample_ms[ch]=now;
        }
    }
    polling_wpad=NULL;
}
