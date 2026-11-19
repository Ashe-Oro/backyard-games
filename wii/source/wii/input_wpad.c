#include "input_wpad.h"
#include <math.h>
#include <string.h>

_Static_assert(INPUT_A==WPAD_BUTTON_A && INPUT_B==WPAD_BUTTON_B &&
               INPUT_ONE==WPAD_BUTTON_1 && INPUT_TWO==WPAD_BUTTON_2 &&
               INPUT_HOME==WPAD_BUTTON_HOME && INPUT_MINUS==WPAD_BUTTON_MINUS &&
               INPUT_PLUS==WPAD_BUTTON_PLUS && INPUT_LEFT==WPAD_BUTTON_LEFT &&
               INPUT_RIGHT==WPAD_BUTTON_RIGHT && INPUT_UP==WPAD_BUTTON_UP &&
               INPUT_DOWN==WPAD_BUTTON_DOWN, "WPAD button map changed");

static InputWpad *s_wpad;

int input_wpad_init(InputWpad *w, Input *input, InputWpadTraceSink trace, void *user) {
    memset(w,0,sizeof(*w));
    w->input=input; w->trace=trace; w->trace_user=user;
    w->vres_x=640; w->vres_y=480;
    for (unsigned ch=0;ch<2;++ch)
        for (unsigned axis=0;axis<3;++axis) w->raw_high[ch][axis]=1023;
    s_wpad=w;
    return WPAD_Init();
}
void input_wpad_set_vres(InputWpad *w, uint32_t xres, uint32_t yres) {
    if (!w || !xres || !yres) return;
    w->vres_x=xres; w->vres_y=yres;
    WPAD_SetVRes(WPAD_CHAN_ALL,xres,yres);
    for (unsigned ch=0;ch<2;++ch)
        if (w->registered[ch]) WPAD_SetVRes((int)ch,xres,yres);
}
bool input_wpad_pointer(const InputWpad *w, unsigned ch, InputPointer *out) {
    if (!w || ch>=2 || !w->pointer[ch].valid) return false;
    if (out) *out=w->pointer[ch];
    return true;
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
    /* ACC+IR so a sensor bar can draw a cursor; buttons and accel must not wait on IR. */
    int rc=WPAD_SetDataFormat((int)ch,WPAD_FMT_BTNS_ACC_IR);
    if (rc<0) return rc;
    rc=WPAD_SetVRes((int)ch,w->vres_x,w->vres_y);
    if (rc<0) return rc;
    /* Continuous format retains reports; zero thresholds also retain small idle changes. */
    rc=WPAD_SetIdleThresholds((int)ch,WPAD_THRESH_ANY,WPAD_THRESH_ANY,WPAD_THRESH_ANY,
                            WPAD_THRESH_IGNORE,WPAD_THRESH_IGNORE,WPAD_THRESH_IGNORE);
    if (rc<0) return rc;
    return WPAD_SetEventBufs((int)ch,w->events[ch],INPUT_WPAD_EVENTS);
}
static void lost_channel(InputWpad *w, unsigned ch) {
    w->registered[ch]=w->have_sample[ch]=w->have_event_path[ch]=false;
    w->pointer[ch].valid=false;
    input_connection(w->input,ch,false);
}
static void update_pointer(InputWpad *w, unsigned ch, const WPADData *d) {
    InputPointer *p=&w->pointer[ch];
    /* ir.valid is optional. Never require it for buttons or play. */
    if (!d || d->err!=WPAD_ERR_NONE || (d->data_present&WPAD_DATA_IR)==0 || !d->ir.valid) {
        p->valid=false;
        return;
    }
    float vx=w->vres_x ? (float)w->vres_x : 640.f;
    float vy=w->vres_y ? (float)w->vres_y : 480.f;
    p->x=d->ir.x*(640.f/vx);
    p->y=d->ir.y*(480.f/vy);
    p->valid=true;
}
static void feed_report(InputWpad *w, unsigned ch, WPADData *d, unsigned n, unsigned drops,
                        uint64_t first, bool reject, uint64_t now) {
    Input *i=w->input;
    InputSample sample={0};
    sample.motion.ms=first+(uint64_t)n*INPUT_WPAD_REPORT_MS;
    sample.motion.g=(SwingVector){d->gforce.x,d->gforce.y,d->gforce.z};
    /* Accept buttons whenever the report is clean. Some real-remote paths leave data_present
     * incomplete after ScanPads; do not wait on IR or a buttons bit. */
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
    if (!reject) input_feed(i,ch,&sample);
    update_pointer(w,ch,d);
    if (w->trace) {
        InputWpadTrace row={ch,n,i->active_slot,now,sample.motion.ms,
            sample.motion.dropped,true,reject,d,&sample,&i->detector};
        w->trace(w->trace_user,&row);
    }
}
void input_wpad_poll(InputWpad *w, uint64_t now) {
    bool stall=w->have_poll && (now<=w->last_poll_ms || now-w->last_poll_ms>SWING_GAP_MS);
    w->have_poll=true; w->last_poll_ms=now;
    Input *i=w->input;
    if (stall) input_report_loss(i);
    /* Pump the Bluetooth/WPAD stack every frame. Event-buffer-only homebrew can see remotes
     * as connected (Probe succeeds) while ReadEvent stays empty on real DolphinBar remotes. */
    WPAD_ScanPads();
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
        bool from_scan=false;
        if (count>0) w->have_event_path[ch]=true;
        else {
            /* ScanPads snapshot: used when the event queue is empty (DolphinBar and many
             * emulated paths). If the event path already produced reports, only feed the
             * snapshot when it actually changed, so a silent queue is still a swing gap. */
            WPADData *cur=WPAD_Data((int)ch);
            if (cur && cur->err==WPAD_ERR_NONE) {
                if (!cur->btns_h) cur->btns_h=WPAD_ButtonsHeld((int)ch);
                bool changed=!w->have_sample[ch] || cur->btns_h!=w->last_btns[ch] ||
                    cur->gforce.x!=w->last_gforce[ch].x || cur->gforce.y!=w->last_gforce[ch].y ||
                    cur->gforce.z!=w->last_gforce[ch].z;
                if (!w->have_event_path[ch] || changed) {
                    w->batch[0]=*cur;
                    count=1;
                    from_scan=true;
                }
            }
        }
        /* This API returns the count since its previous read and clears it. */
        int drops=WPAD_DroppedEvents((int)ch);
        if (drops<0) { w->status[ch]=drops; lost_channel(w,ch); continue; }
        w->dropped[ch]+=(unsigned)drops;
        w->reports[ch]+=count;
        uint64_t width=(!from_scan && count) ? (uint64_t)(count-1)*INPUT_WPAD_REPORT_MS : 0;
        bool reject=false;
        if (!from_scan) {
            reject=stall || drops>0 || count==INPUT_WPAD_EVENTS || width>now;
            uint64_t first_chk=width<=now ? now-width : 0;
            if (count && w->have_sample[ch] && (first_chk<=w->last_sample_ms[ch] ||
                first_chk-w->last_sample_ms[ch]>SWING_GAP_MS)) reject=true;
        }
        uint64_t first=width<=now ? now-width : 0;
        if (reject) {
            ++w->rejected_batches[ch];
            if (ch==i->channels[i->active_slot]) input_report_loss(i);
        }
        if (!count && w->have_sample[ch] && now-w->last_sample_ms[ch]>SWING_GAP_MS &&
            ch==i->channels[i->active_slot]) input_report_loss(i);
        for (unsigned n=0;n<count;++n)
            feed_report(w,ch,&w->batch[n],n,(unsigned)drops,first,reject,now);
        if (count) {
            w->have_sample[ch]=true; w->last_sample_ms[ch]=now;
            w->last_btns[ch]=w->batch[count-1].btns_h;
            w->last_gforce[ch].x=w->batch[count-1].gforce.x; w->last_gforce[ch].y=w->batch[count-1].gforce.y; w->last_gforce[ch].z=w->batch[count-1].gforce.z;
        }
    }
}
