#include "../source/wii/input_wpad.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    bool connected;
    WPADData queue[128];
    unsigned count, read, registrations, drops;
    int fail_stage;
    WPADData *registered_buffer;
} FakeRemote;
static FakeRemote remotes[2];
static unsigned tests, traced, rejected, pauses;
static uint64_t trace_ms[128];
static InputWpadTrace last_trace;
static InputSample last_sample;
static int init_status;

int32_t WPAD_Init(void) { return init_status; }
int32_t WPAD_Probe(int32_t ch, uint32_t *type) {
    assert(ch>=0 && ch<2); *type=0;
    return remotes[ch].connected ? 0 : WPAD_ERR_NO_CONTROLLER;
}
int32_t WPAD_SetDataFormat(int32_t ch, int32_t format) {
    assert(format==WPAD_FMT_BTNS_ACC_IR); return remotes[ch].fail_stage==1 ? -3 : 0;
}
int32_t WPAD_SetVRes(int32_t ch, uint32_t x, uint32_t y) {
    assert(x==640 && y==480); return remotes[ch].fail_stage==2 ? -3 : 0;
}
int32_t WPAD_SetIdleThresholds(int32_t ch, int32_t btn, int32_t ir, int32_t acc,
                               int32_t js, int32_t wb, int32_t mp) {
    assert(btn==0 && ir==0 && acc==0 && js==-1 && wb==-1 && mp==-1);
    return remotes[ch].fail_stage==3 ? -3 : 0;
}
int32_t WPAD_SetEventBufs(int32_t ch, WPADData *buf, uint32_t count) {
    assert(count==64 && buf);
    if (remotes[ch].fail_stage==4) return -3;
    remotes[ch].registered_buffer=buf; ++remotes[ch].registrations;
    return 0;
}
int32_t WPAD_ReadEvent(int32_t ch, WPADData *data) {
    FakeRemote *r=&remotes[ch];
    if (r->fail_stage==5) return WPAD_ERR_TRANSFER;
    if (r->read==r->count) return WPAD_ERR_QUEUE_EMPTY;
    *data=r->queue[r->read++]; return 0;
}
int32_t WPAD_DroppedEvents(int32_t ch) {
    if (remotes[ch].fail_stage==6) return WPAD_ERR_TRANSFER;
    unsigned n=remotes[ch].drops; remotes[ch].drops=0; return (int32_t)n;
}
static void event(void *user, const InputEvent *e) {
    (void)user; if (e->action==INPUT_CONNECT_REMOTE) ++pauses;
}
static void trace(void *user, const InputWpadTrace *row) {
    (void)user;
    assert(row->raw && row->sample && row->detector && row->timestamp_estimated);
    assert(traced<128); trace_ms[traced++]=row->estimated_ms;
    rejected+=row->batch_rejected;
    last_trace=*row; last_trace.raw=NULL; last_trace.sample=NULL; last_trace.detector=NULL;
    last_sample=*row->sample;
}
static void setup(Input *i, InputWpad *w, bool two) {
    memset(remotes,0,sizeof(remotes));
    traced=rejected=pauses=0; init_status=0;
    remotes[0].connected=true; remotes[1].connected=two;
    input_init(i,event,NULL); assert(input_wpad_init(w,i,trace,NULL)==0);
    input_wpad_poll(w,1000); input_start_match(i);
}
static void queue(unsigned ch, unsigned count, uint32_t buttons) {
    FakeRemote *r=&remotes[ch]; r->count=count; r->read=0;
    for (unsigned n=0;n<count;++n) r->queue[n]=(WPADData){
        .data_present=WPAD_DATA_BUTTONS|WPAD_DATA_ACCEL|WPAD_DATA_IR,.btns_h=buttons,
        .ir={1,480,240},.accel={500,500,700},.gforce={0,0,1}};
}
static void both_channels_drained_in_order(void) {
    Input i; InputWpad w; setup(&i,&w,true);
    assert(remotes[0].registered_buffer==w.events[0] && remotes[1].registered_buffer==w.events[1]);
    queue(0,2,0); queue(1,2,0); input_wpad_poll(&w,1020);
    assert(traced==4 && !rejected && remotes[0].read==2 && remotes[1].read==2);
    assert(trace_ms[0]==1010 && trace_ms[1]==1020 && trace_ms[2]==1010 && trace_ms[3]==1020);
    assert(w.reports[0]==2 && w.reports[1]==2);
    assert(fabs(i.slots[0].aim-.06)<1e-9 && !i.b_held[0] && !i.b_held[1]);
}
static void reconnect_registers_persistent_buffer(void) {
    Input i; InputWpad w; setup(&i,&w,true); input_next_turn(&i,1);
    i.detector.phase=SWING_GRIP;
    remotes[1].connected=false; input_wpad_poll(&w,1010);
    assert(pauses==1 && i.paused && !swing_armed(&i.detector) && !w.registered[1]);
    remotes[1].connected=true; queue(1,1,INPUT_B); input_wpad_poll(&w,1020);
    assert(remotes[1].registrations==2 && remotes[1].registered_buffer==w.events[1]);
    assert(!input_resume(&i)); queue(1,1,0); input_wpad_poll(&w,1030); assert(input_resume(&i));
}
static void check_registration_and_read_errors(void) {
    for (int failure=1;failure<=6;++failure) {
        Input i; InputWpad w; setup(&i,&w,false);
        w.registered[0]=false; remotes[0].fail_stage=failure; input_wpad_poll(&w,1010);
        assert(w.status[0]==WPAD_ERR_TRANSFER && !w.registered[0] && !i.connected[0]);
    }
    Input i; InputWpad w; input_init(&i,NULL,NULL); init_status=-2;
    assert(input_wpad_init(&w,&i,NULL,NULL)==-2); init_status=0;
}
static void dropped_events_are_counts_not_cumulative(void) {
    Input i; InputWpad w; setup(&i,&w,false); i.detector.phase=SWING_GRIP;
    queue(0,2,INPUT_B); remotes[0].drops=3; input_wpad_poll(&w,1020);
    assert(rejected==2 && w.dropped[0]==3 && !swing_armed(&i.detector));
    assert(last_trace.batch_rejected && w.rejected_batches[0]==1);
    queue(0,1,0); input_wpad_poll(&w,1030);
    assert(w.dropped[0]==3 && !last_trace.batch_rejected);
}
static void stall_and_packet_gap_cancel(void) {
    Input i; InputWpad w; setup(&i,&w,false);
    queue(0,1,INPUT_B); input_wpad_poll(&w,1010); i.detector.phase=SWING_GRIP;
    queue(0,4,INPUT_B); input_wpad_poll(&w,1050);
    assert(rejected==4 && !swing_armed(&i.detector));
    setup(&i,&w,false); queue(0,1,INPUT_B); input_wpad_poll(&w,1010);
    i.detector.phase=SWING_GRIP; queue(0,0,0);
    input_wpad_poll(&w,1030); input_wpad_poll(&w,1040);
    assert(!swing_armed(&i.detector));
}
static void invalid_data_and_clipping_flags(void) {
    Input i; InputWpad w; setup(&i,&w,false);
    const uint16_t lo[]={10,20,30}, hi[]={900,910,920};
    assert(input_wpad_set_rails(&w,0,lo,hi));
    assert(!input_wpad_set_rails(&w,2,lo,hi));
    queue(0,1,INPUT_B); remotes[0].queue[0].accel.x=899;
    input_wpad_poll(&w,1010); assert(last_sample.motion.clipped);
    queue(0,1,0); remotes[0].queue[0].data_present=WPAD_DATA_BUTTONS;
    i.detector.phase=SWING_GRIP; input_wpad_poll(&w,1020);
    assert(!last_sample.motion.accel_valid && !last_sample.motion.ir && !swing_armed(&i.detector));
    queue(0,1,INPUT_A); remotes[0].queue[0].err=WPAD_ERR_TRANSFER;
    input_wpad_poll(&w,1030); assert(!last_sample.buttons_valid && !i.slots[0].sticky);
}
static void overlapping_timestamps_and_full_buffer_rejected(void) {
    Input i; InputWpad w; setup(&i,&w,false);
    queue(0,1,0); input_wpad_poll(&w,1010);
    queue(0,3,INPUT_B); input_wpad_poll(&w,1020);
    assert(rejected==3 && last_trace.batch_rejected);
    setup(&i,&w,false); queue(0,64,INPUT_B); input_wpad_poll(&w,1020);
    assert(rejected==64 && w.rejected_batches[0]==1);
}
#define TEST(fn) do { fn(); ++tests; printf("ok %u - %s\n",tests,#fn); } while (0)
int main(void) {
    TEST(both_channels_drained_in_order); TEST(reconnect_registers_persistent_buffer);
    TEST(check_registration_and_read_errors); TEST(dropped_events_are_counts_not_cumulative);
    TEST(stall_and_packet_gap_cancel); TEST(invalid_data_and_clipping_flags);
    TEST(overlapping_timestamps_and_full_buffer_rejected);
    printf("%u WPAD adapter tests passed\n",tests); return 0;
}
