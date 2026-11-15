#ifndef TEST_FAKE_WPAD_H
#define TEST_FAKE_WPAD_H
/* Test-owned API double, NOT libogc and never on the Wii include path.
   Only the fields consumed by input_wpad.c are represented. */
#include <stdint.h>
#define WPAD_BUTTON_2 0x0001
#define WPAD_BUTTON_1 0x0002
#define WPAD_BUTTON_B 0x0004
#define WPAD_BUTTON_A 0x0008
#define WPAD_BUTTON_MINUS 0x0010
#define WPAD_BUTTON_HOME 0x0080
#define WPAD_BUTTON_LEFT 0x0100
#define WPAD_BUTTON_RIGHT 0x0200
#define WPAD_BUTTON_DOWN 0x0400
#define WPAD_BUTTON_UP 0x0800
#define WPAD_BUTTON_PLUS 0x1000
#define WPAD_FMT_BTNS_ACC_IR 2
#define WPAD_ERR_NONE 0
#define WPAD_ERR_NO_CONTROLLER -1
#define WPAD_ERR_NOT_READY -2
#define WPAD_ERR_TRANSFER -3
#define WPAD_ERR_QUEUE_EMPTY -7
#define WPAD_DATA_BUTTONS 1
#define WPAD_DATA_ACCEL 2
#define WPAD_DATA_IR 8
#define WPAD_THRESH_ANY 0
#define WPAD_THRESH_IGNORE -1
typedef struct {
    int16_t err;
    uint32_t data_present, btns_h;
    struct { int valid; float x,y; } ir;
    struct { uint16_t x,y,z; } accel;
    struct { float x,y,z; } gforce;
    struct { int type; } exp;
} WPADData;
int32_t WPAD_Init(void);
int32_t WPAD_Probe(int32_t channel, uint32_t *type);
int32_t WPAD_SetDataFormat(int32_t channel, int32_t format);
int32_t WPAD_SetVRes(int32_t channel, uint32_t x, uint32_t y);
int32_t WPAD_SetIdleThresholds(int32_t channel, int32_t buttons, int32_t ir, int32_t accel,
                               int32_t js, int32_t wb, int32_t mp);
int32_t WPAD_SetEventBufs(int32_t channel, WPADData *buffer, uint32_t count);
int32_t WPAD_ReadEvent(int32_t channel, WPADData *data);
int32_t WPAD_DroppedEvents(int32_t channel);
#endif
