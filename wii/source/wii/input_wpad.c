#include "input_wpad.h"
#include <math.h>
#include <string.h>

_Static_assert(INPUT_A==WPAD_BUTTON_A && INPUT_B==WPAD_BUTTON_B &&
               INPUT_ONE==WPAD_BUTTON_1 && INPUT_TWO==WPAD_BUTTON_2 &&
               INPUT_HOME==WPAD_BUTTON_HOME && INPUT_MINUS==WPAD_BUTTON_MINUS &&
               INPUT_PLUS==WPAD_BUTTON_PLUS && INPUT_LEFT==WPAD_BUTTON_LEFT &&
               INPUT_RIGHT==WPAD_BUTTON_RIGHT && INPUT_UP==WPAD_BUTTON_UP &&
               INPUT_DOWN==WPAD_BUTTON_DOWN, "WPAD button map changed");

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
    int rc=WPAD_SetDataFormat((int)ch,WPAD_FMT_BTNS_ACC_IR);
    if (rc<0) return rc;
    rc=WPAD_SetVRes((int)ch,INPUT_WPAD_WIDTH,INPUT_WPAD_HEIGHT);
    if (rc<0) return rc;
    /* Continuous format retains reports; zero thresholds also retain small idle changes. */
    rc=WPAD_SetIdleThresholds((int)ch,WPAD_THRESH_ANY,WPAD_THRESH_ANY,WPAD_THRESH_ANY,
                            WPAD_THRESH_IGNORE,WPAD_THRESH_IGNORE,WPAD_THRESH_IGNORE);
    if (rc<0) return rc;
    return WPAD_SetEventBufs((int)ch,w->events[ch],INPUT_WPAD_EVENTS);
}
static void lost_channel(InputWpad *w, unsigned ch) {
    w->registered[ch]=w->have_sample[ch]=false;
    input_connection(w->input,ch,false);
}
void input_wpad_poll(InputWpad *w, uint64_t now) {
    bool stall=w->have_poll && (now<=w->last_poll_ms || now-w->last_poll_ms>SWING_GAP_MS);
    w->have_poll=true; w->last_poll_ms=now;
    Input *i=w->input;
    if (stall) input_report_loss(i);
    for (unsigned ch=0;ch<2;++ch) {
        uint32_t type;
        int rc=WPAD_Probe((int)ch,&type);
        w->status[ch]=rc;
        if (rc<0) { lost_channel(w,ch); continue; }
        if (!w->registered[ch]) {
            rc=register_channel(w,ch);
            w->status[ch]=rc;
            if (rc<0) { lost_channel(w,ch); continue; }
            w->registered[ch]=true;
            input_connection(i,ch,true);
        }
        unsigned count=0;
        while (count<INPUT_WPAD_EVENTS && (rc=WPAD_ReadEvent((int)ch,&w->batch[count]))>=0) ++count;
        if (rc<0 && rc!=WPAD_ERR_QUEUE_EMPTY) {
            w->status[ch]=rc;
            lost_channel(w,ch);
            continue;
        }
        /* This API returns the count since its previous read and clears it. */
        int drops=WPAD_DroppedEvents((int)ch);
        if (drops<0) { w->status[ch]=drops; lost_channel(w,ch); continue; }
        w->dropped[ch]+=(unsigned)drops;
        w->reports[ch]+=count;
        uint64_t width=count ? (uint64_t)(count-1)*INPUT_WPAD_REPORT_MS : 0;
        bool reject=stall || drops>0 || count==INPUT_WPAD_EVENTS || width>now;
        uint64_t first=width<=now ? now-width : 0;
        if (count && w->have_sample[ch] && (first<=w->last_sample_ms[ch] ||
            first-w->last_sample_ms[ch]>SWING_GAP_MS)) reject=true;
        if (reject) {
            ++w->rejected_batches[ch];
            if (ch==i->channels[i->active_slot]) input_report_loss(i);
        }
        if (!count && w->have_sample[ch] && now-w->last_sample_ms[ch]>SWING_GAP_MS &&
            ch==i->channels[i->active_slot]) input_report_loss(i);
        for (unsigned n=0;n<count;++n) {
            WPADData *d=&w->batch[n];
            InputSample sample={0};
            sample.motion.ms=first+(uint64_t)n*INPUT_WPAD_REPORT_MS;
            sample.motion.g=(SwingVector){d->gforce.x,d->gforce.y,d->gforce.z};
            sample.motion.accel_valid=d->err==WPAD_ERR_NONE && (d->data_present&WPAD_DATA_ACCEL)!=0;
            sample.buttons_valid=d->err==WPAD_ERR_NONE && (d->data_present&WPAD_DATA_BUTTONS)!=0;
            sample.buttons=d->btns_h;
            sample.motion.b=(sample.buttons&INPUT_B)!=0;
            sample.motion.ir=d->err==WPAD_ERR_NONE && (d->data_present&WPAD_DATA_IR)!=0 &&
                             d->ir.valid && isfinite(d->ir.x) && isfinite(d->ir.y);
            sample.pointer_x=d->ir.x/(double)INPUT_WPAD_WIDTH;
            sample.pointer_y=d->ir.y/(double)INPUT_WPAD_HEIGHT;
            sample.motion.dropped=n==0 ? (unsigned)drops : 0;
            const uint16_t raw[]={d->accel.x,d->accel.y,d->accel.z};
            for (unsigned a=0;a<3;++a)
                if (raw[a]<=w->raw_low[ch][a]+2 || raw[a]>=w->raw_high[ch][a]-2)
                    sample.motion.clipped=true;
            if (!reject) input_feed(i,ch,&sample);
            if (w->trace) {
                InputWpadTrace row={ch,n,i->active_slot,now,sample.motion.ms,
                    sample.motion.dropped,true,reject,d,&sample,&i->detector};
                w->trace(w->trace_user,&row);
            }
        }
        if (count) { w->have_sample[ch]=true; w->last_sample_ms[ch]=now; }
    }
}
