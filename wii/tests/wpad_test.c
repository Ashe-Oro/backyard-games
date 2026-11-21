#include "../source/wii/input_wpad.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    bool connected;
    WPADData queue[128], snapshot;
    unsigned count, read, registrations, drops, scans;
    int fail_stage, format;
    uint32_t vres_x, vres_y;
    WPADData *registered_buffer;
} FakeRemote;
static unsigned scan_pads, throws;
static bool scan_consumes;
static SwingResult shot;
static FakeRemote remotes[2];
static unsigned tests, traced, rejected, pauses;
static uint64_t trace_ms[512];
static InputWpadTrace last_trace;
static InputSample last_sample;
static int init_status;

int32_t WPAD_Init(void) { return init_status; }
int32_t WPAD_Probe(int32_t ch, uint32_t *type) {
    assert(ch>=0 && ch<2); *type=0;
    return remotes[ch].connected ? 0 : WPAD_ERR_NO_CONTROLLER;
}
int32_t WPAD_SetDataFormat(int32_t ch, int32_t format) {
    assert(format==WPAD_FMT_BTNS_ACC_IR);
    remotes[ch].format=format;
    return remotes[ch].fail_stage==1 ? -3 : 0;
}
int32_t WPAD_SetVRes(int32_t ch, uint32_t xres, uint32_t yres) {
    if (ch==WPAD_CHAN_ALL) {
        remotes[0].vres_x=remotes[1].vres_x=xres;
        remotes[0].vres_y=remotes[1].vres_y=yres;
        return 0;
    }
    assert(ch>=0 && ch<2);
    remotes[ch].vres_x=xres; remotes[ch].vres_y=yres;
    return remotes[ch].fail_stage==2 ? -3 : 0;
}
/* In libogc 3.2.1 ScanPads is ReadPending(ALL, NULL). Its drain
 * runs the callback before coalescing the frame-level held state. */
int32_t WPAD_ReadPending(int32_t ch, void (*callback)(int32_t, const WPADData *)) {
    FakeRemote *r=&remotes[ch];
    unsigned count=0;
    uint32_t held=r->snapshot.btns_h, previous=held, edges=0;
    int32_t rc;
    while ((rc=WPAD_ReadEvent(ch,&r->snapshot))>=0) {
        if (callback) callback(ch,&r->snapshot);
        uint32_t current=r->snapshot.btns_h;
        uint32_t changed=(current^previous)&~edges;
        held=(held&~changed)|(current&changed);
        edges|=changed;
        previous=current;
        ++count;
    }
    r->snapshot.btns_h=held;
    return rc==WPAD_ERR_QUEUE_EMPTY ? (int32_t)count : rc;
}
int32_t WPAD_ScanPads(void) {
    ++scan_pads;
    for (int ch=0;ch<2;++ch) {
        ++remotes[ch].scans;
        if (scan_consumes && remotes[ch].connected) WPAD_ReadPending(ch,NULL);
    }
    return 0;
}
WPADData *WPAD_Data(int ch) {
    assert(ch>=0 && ch<2);
    return remotes[ch].connected ? &remotes[ch].snapshot : NULL;
}
uint32_t WPAD_ButtonsHeld(int ch) {
    assert(ch>=0 && ch<2);
    return remotes[ch].snapshot.btns_h;
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
    (void)user;
    if (e->action==INPUT_CONNECT_REMOTE) ++pauses;
    if (e->action==INPUT_THROW) { ++throws; shot=e->toss; }
}
static void trace(void *user, const InputWpadTrace *row) {
    (void)user;
    assert(row->raw && row->sample && row->detector && row->timestamp_estimated);
    assert(traced<512); trace_ms[traced++]=row->estimated_ms;
    rejected+=row->batch_rejected;
    last_trace=*row; last_trace.raw=NULL; last_trace.sample=NULL; last_trace.detector=NULL;
    last_sample=*row->sample;
}
static void setup(Input *i, InputWpad *w, bool two) {
    memset(remotes,0,sizeof(remotes));
    traced=rejected=pauses=throws=0; scan_consumes=false; init_status=0; scan_pads=0;
    remotes[0].connected=true; remotes[1].connected=two;
    remotes[0].snapshot.err=WPAD_ERR_NO_CONTROLLER;
    remotes[1].snapshot.err=WPAD_ERR_NO_CONTROLLER;
    input_init(i,event,NULL); assert(input_wpad_init(w,i,trace,NULL)==0);
    input_wpad_poll(w,1000); input_start_match(i);
}
static void queue(unsigned ch, unsigned count, uint32_t buttons) {
    FakeRemote *r=&remotes[ch]; r->count=count; r->read=0;
    for (unsigned n=0;n<count;++n) r->queue[n]=(WPADData){
        .data_present=WPAD_DATA_BUTTONS|WPAD_DATA_ACCEL,.btns_h=buttons,
        .accel={500,500,700},.gforce={0,0,1}};
}
static void both_channels_drained_in_order(void) {
    Input i; InputWpad w; setup(&i,&w,true);
    assert(remotes[0].registered_buffer==w.events[0] && remotes[1].registered_buffer==w.events[1]);
    queue(0,2,0); queue(1,2,0); input_wpad_poll(&w,1020);
    assert(traced==4 && !rejected && remotes[0].read==2 && remotes[1].read==2);
    assert(trace_ms[0]==1010 && trace_ms[1]==1020 && trace_ms[2]==1010 && trace_ms[3]==1020);
    assert(w.reports[0]==2 && w.reports[1]==2);
    assert(fabs(i.slots[0].aim)<1e-9 && !i.b_held[0] && !i.b_held[1]);
    assert(scan_pads>=2 && remotes[0].format==WPAD_FMT_BTNS_ACC_IR);
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
    const int failures[]={1,3,4,5,6};
    for (unsigned n=0;n<sizeof(failures)/sizeof(failures[0]);++n) {
        int failure=failures[n];
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
    assert(!last_sample.motion.accel_valid && !swing_armed(&i.detector));
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

static void scan_fallback_and_optional_ir(void) {
    Input i; InputWpad w; setup(&i,&w,false);
    input_set_context(&i,INPUT_MENU);
    /* Event queue stays empty; ScanPads snapshot still delivers A. */
    remotes[0].snapshot=(WPADData){.err=WPAD_ERR_NONE,
        .data_present=WPAD_DATA_BUTTONS,.btns_h=INPUT_A,
        .accel={500,500,700},.gforce={0,0,1}};
    unsigned before=traced;
    input_wpad_poll(&w,1010);
    assert(traced==before+1 && last_sample.buttons_valid && last_sample.buttons==INPUT_A);
    assert(!w.have_event_path[0] && remotes[0].scans>=1);
    InputPointer ptr;
    assert(!input_wpad_pointer(&w,0,&ptr));
    /* IR cursor is optional and never required for the next button. */
    remotes[0].snapshot.ir.valid=1; remotes[0].snapshot.ir.x=320; remotes[0].snapshot.ir.y=240;
    remotes[0].snapshot.data_present|=WPAD_DATA_IR;
    remotes[0].snapshot.btns_h=INPUT_TWO;
    input_wpad_poll(&w,1020);
    assert(input_wpad_pointer(&w,0,&ptr) && fabs(ptr.x-320)<1e-3 && fabs(ptr.y-240)<1e-3);
    assert(last_sample.buttons_valid && last_sample.buttons==INPUT_TWO);
    remotes[0].snapshot.ir.valid=0;
    input_wpad_poll(&w,1030);
    assert(!input_wpad_pointer(&w,0,&ptr) && last_sample.buttons_valid);
    input_wpad_set_vres(&w,1280,960);
    assert(remotes[0].vres_x==1280 && remotes[0].vres_y==960);
}
static void event_path_does_not_replay_stale_snapshot(void) {
    /* Once ReadEvent works, an empty queue is a gap, not a WPAD_Data replay. */
    Input i; InputWpad w; setup(&i,&w,false);
    queue(0,1,INPUT_B); remotes[0].snapshot=remotes[0].queue[0];
    input_wpad_poll(&w,1010);
    assert(w.have_event_path[0] && last_sample.buttons==INPUT_B);
    unsigned before=traced;
    input_wpad_poll(&w,1020);
    assert(traced==before);
}
/* Recorded fixtures are constructed from a 10 ms bowling gesture, with
 * a moving backswing apex and a radial load through the forward pass.
 * These are modeled samples, not captures from a physical Remote.
 * Calibration is 128 at 0 g and 154 at +1 g (high bytes). The 10-bit
 * WPAD readings carry two low bits on X and one on Y/Z. The modeled
 * forces stay inside the documented +/-3 g range on every axis.
 * https://wiibrew.org/wiki/Wiimote#Accelerometer
 * https://github.com/devkitPro/libogc/blob/v3.2.1/wiiuse/dynamics.c */
typedef struct { double pitch, excess, lateral; } SwingKey;
static const SwingKey back_keys[]={
    {0,0,0}, {-5,.08,0}, {-13,.24,0}, {-23,.29,0}, {-33,.18,0},
    {-43,.08,0}, {-46,0,0}, {-43,.01,0}
};
static const SwingKey forward_keys[]={
    {-43,.01,0}, {-36,.11,.04}, {-26,.36,.12}, {-15,.72,.14},
    {-4,1.03,.16}, {9,1.07,.18}, {22,.83,.16}, {34,.52,.13}, {42,.26,.12}
};
static WPADData realistic_report(unsigned frame, unsigned release, double strength, bool tap) {
    double pitch=0, excess=0, lateral=0;
    if (!tap && frame>=59) {
        bool back=frame<80;
        const SwingKey *keys=back ? back_keys : forward_keys;
        unsigned n=back ? 8 : 9;
        unsigned tick=back ? frame-59 : frame-80;
        unsigned k=tick/3;
        if (k>=n-1) {
            pitch=keys[n-1].pitch;
            lateral=keys[n-1].lateral;
        } else {
            double t=(tick%3)/3.0;
            pitch=keys[k].pitch+(keys[k+1].pitch-keys[k].pitch)*t;
            excess=keys[k].excess+(keys[k+1].excess-keys[k].excess)*t;
            lateral=keys[k].lateral+(keys[k+1].lateral-keys[k].lateral)*t;
        }
    }
    double radians=pitch*3.14159265358979323846/180;
    double mag=1+strength*excess;
    /* Small measurement jitter persists when the remote is otherwise still. */
    uint16_t x=(uint16_t)lround(512+104*lateral+(int)(frame%3)-1);
    uint16_t y=(uint16_t)(2*lround((512+104*mag*sin(radians))/2));
    uint16_t z=(uint16_t)(2*lround((512+104*mag*cos(radians))/2));
    return (WPADData){.data_present=WPAD_DATA_BUTTONS|WPAD_DATA_ACCEL,
        .btns_h=frame>=53 && frame<release ? INPUT_B : 0, .accel={x,y,z},
        .gforce={(x-512)/104.f,(y-512)/104.f,(z-512)/104.f}};
}
static unsigned replay_realistic(unsigned release, double strength, bool left, bool tap,
                                 bool batched) {
    Input i; InputWpad w; setup(&i,&w,false);
    i.slots[0].profile.left_handed=left;
    scan_consumes=batched;
    unsigned time=0, poll=0;
    while (time<150) {
        unsigned elapsed=batched ? (++poll*50)/3 : (time+1)*10;
        /* At 60 Hz the 100 Hz stream arrives in one- and two-report batches.
         * One three-report burst models normal Bluetooth delivery jitter. */
        unsigned reports=elapsed/10-time;
        if (batched && poll==47) ++reports;
        queue(0,reports,0);
        for (unsigned n=0;n<reports;++n)
            remotes[0].queue[n]=realistic_report(time+n,release,strength,tap);
        time+=reports;
        input_wpad_poll(&w,1000+elapsed);
    }
    if (throws && (shot.release!=SWING_B_UP || shot.release_ms<1000+release*10-15))
        return 99;
    if (tap && i.detector.phase!=SWING_READY) return 99;
    return throws;
}
static void pending_drain_preserves_edges_and_motion(void) {
    Input i; InputWpad w; setup(&i,&w,false);
    scan_consumes=true;
    queue(0,1,0);
    input_wpad_poll(&w,1017);
    unsigned before=traced;
    queue(0,2,0);
    remotes[0].queue[0].btns_h=INPUT_B;
    remotes[0].queue[0].gforce.z=1.2f;
    input_wpad_poll(&w,1034);
    assert(traced==before+2 && rejected==0 && !i.b_held[0]);
    assert(trace_ms[before]>1017 && trace_ms[before+1]==1034);
}
static void realistic_wiimote_releases(void) {
    static const struct {
        const char *name; unsigned release; double strength; bool left;
    } cases[]={
        {"normal",97,.80,false}, {"gentle",99,.22,false}, {"fast",95,1.5,false},
        {"lefty",97,.80,true}, {"end-of-forward",104,.9,false},
        {"short",88,.50,false}
    };
    unsigned passed=0;
    double powers[sizeof(cases)/sizeof(cases[0])];
    for (unsigned n=0;n<sizeof(cases)/sizeof(cases[0]);++n) {
        unsigned uniform=replay_realistic(cases[n].release,cases[n].strength,cases[n].left,false,false);
        unsigned batched=replay_realistic(cases[n].release,cases[n].strength,cases[n].left,false,true);
        powers[n]=batched==1 ? shot.power : 0;
        printf("trace %s: %u uniform, %u batched throws (power %.3f)\n",
               cases[n].name,uniform,batched,powers[n]);
        passed+=uniform==1 && batched==1;
        if (batched==1) assert(cases[n].left ? shot.aim<0 : shot.aim>0);
    }
    unsigned tap=replay_realistic(56,0,false,true,true);
    printf("trace B tap: %u throws\n",tap);
    fflush(stdout);
    assert(passed==sizeof(cases)/sizeof(cases[0]) && tap==0);
    assert(powers[1]<powers[0] && powers[0]<powers[2] && powers[5]<powers[0]);
}
#define TEST(fn) do { fn(); ++tests; printf("ok %u - %s\n",tests,#fn); } while (0)
int main(int argc, char **argv) {
    if (argc==2 && strcmp(argv[1],"realistic")==0) { realistic_wiimote_releases(); return 0; }
    if (argc==2 && strcmp(argv[1],"pending")==0) { pending_drain_preserves_edges_and_motion(); return 0; }
    TEST(both_channels_drained_in_order); TEST(reconnect_registers_persistent_buffer);
    TEST(check_registration_and_read_errors); TEST(dropped_events_are_counts_not_cumulative);
    TEST(stall_and_packet_gap_cancel); TEST(invalid_data_and_clipping_flags);
    TEST(overlapping_timestamps_and_full_buffer_rejected);
    TEST(scan_fallback_and_optional_ir); TEST(event_path_does_not_replay_stale_snapshot);
    TEST(pending_drain_preserves_edges_and_motion); TEST(realistic_wiimote_releases);
    printf("%u WPAD adapter tests passed\n",tests); return 0;
}
