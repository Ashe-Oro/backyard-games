#ifndef BYC_INPUT_WPAD_H
#define BYC_INPUT_WPAD_H

#include <wiiuse/wpad.h>
#include "../core/input.h"

#define INPUT_WPAD_EVENTS 64
#define INPUT_WPAD_REPORT_MS 10 /* nominal spacing for a channel’s first batch */

typedef struct {
    unsigned channel, event_index, player_slot;
    uint64_t drain_ms, estimated_ms;
    unsigned dropped;
    bool timestamp_estimated, batch_rejected;
    const WPADData *raw; /* borrowed for callback only; includes extension, raw axes, err, data_present */
    const InputSample *sample;
    const Swing *detector;
} InputWpadTrace;
typedef void (*InputWpadTraceSink)(void *user, const InputWpadTrace *row);
typedef struct {
    WPADData events[2][INPUT_WPAD_EVENTS]; /* persistent WPAD registrations */
    WPADData batch[INPUT_WPAD_EVENTS];
    unsigned batch_count;
    bool registered[2], have_sample[2], have_event_path[2];
    uint64_t last_sample_ms[2];
    uint16_t raw_low[2][3], raw_high[2][3];
    int status[2];
    uint64_t reports[2], dropped[2], rejected_batches[2];
    InputSample latest[2];
    Input *input;
    InputWpadTraceSink trace;
    void *trace_user;
} InputWpad;

/* Caller owns Input and InputWpad for the application's lifetime. No other WPAD queue consumer. */
int input_wpad_init(InputWpad *wpad, Input *input, InputWpadTraceSink trace, void *user);
/* Call every frame with a monotonic millisecond clock, e.g. ticks_to_millisecs(gettime()). */
void input_wpad_poll(InputWpad *wpad, uint64_t now_ms);
/* Raw per-axis measured rails, including gravity. Defaults 0..1023 are provisional. */
bool input_wpad_set_rails(InputWpad *wpad, unsigned channel, const uint16_t low[3], const uint16_t high[3]);

#endif
