/* Backyard Cornhole — Wii main loop. Owns video/GX, the Wiimote input adapter (wii/input branch),
 * libfat saves and rumble; the game itself is platform-free (source/core/game.c). */
#include <gccore.h>
#include <fat.h>
#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <ogc/lwp_watchdog.h>
#include <wiiuse/wpad.h>

#include "../core/game.h"
#include "../core/save.h"
#include "input_wpad.h"
#include "gfx.h"
#include "scene.h"
#include "hud.h"
#include "music_wii.h"

#define SAVE_DIR "sd:/apps/backyardcornhole/data"
#define SAVE_DAT SAVE_DIR "/save.dat"
#define SAVE_BAK SAVE_DIR "/save.bak"
#define SAVE_TMP SAVE_DIR "/save.tmp"
#define SWING_TRACE SAVE_DIR "/swing-trace.csv"
#define SWING_TRACE_OLD SAVE_DIR "/swing-trace.1.csv"
#define SWING_TRACE_LIMIT (1024 * 1024)
#define SWING_TRACE_FLUSH (16 * 1024)
#define POSE_CAPTURE_MS 300

static Input input;
static InputWpad wpad;
static Game game;
static Music music;
static InputEvent queue[64];
static int queued;
static volatile bool power_off, reset_pressed;
static bool fat_ok;
static int rumble_left[2];
static HudWiimoteCalibration pose_cal;
static unsigned debug_channel, pose_reports;
static uint32_t debug_held[2], debug_blocked[2];
static SwingVector pose_sum;
static uint64_t pose_start_ms, pose_report_ms, static_start_ms, static_report_ms;
static bool static_tracking, static_pose, tracing, trace_sd_failed;
static char trace_buffer[64 * 1024];
static size_t trace_used;
static unsigned trace_dropped;
static const char trace_columns[] =
    "build,report_ms,drain_ms,remote,slot,index,dropped,raw_x,raw_y,raw_z,"
    "accel,buttons,clip,x_g,y_g,z_g,abs_g,phase,radial_g,tangent_g,excess_g,b,reject\n";

static void on_input(void *user, const InputEvent *e) {
    (void)user;
    if (queued < 64) queue[queued++] = *e;   /* applied after input_wpad_poll returns */
}
/* OSReport diagnostics for Dolphin and USB Gecko. */
static void log_event(const InputEvent *e) {
    if (e->action == INPUT_THROW)
        SYS_Report("BYC: throw slot %u power %.3f speed %.2f aim %+.3f spin %+.2f roll %+.1f release %d%s\n", e->slot,
                   e->toss.power, e->toss.speed, e->toss.aim, e->toss.spin, e->toss.roll, (int)e->toss.release, e->toss.clipped ? " clipped" : "");
    else if (e->action == INPUT_CONNECT_REMOTE) SYS_Report("BYC: connect remote %d\n", (int)e->value);
    else if (e->action == INPUT_CAL_DONE) SYS_Report("BYC: SET POWER done slot %u\n", e->slot);
    else SYS_Report("BYC: input action %d value %g\n", (int)e->action, e->value);
}
static void on_reset(u32 irq, void *ctx) { (void)irq; (void)ctx; reset_pressed = true; }
static void on_power(void) { power_off = true; }
static void on_wpad_power(s32 chan) { (void)chan; power_off = true; }

static uint64_t now_ms(void) { return ticks_to_millisecs(gettime()); }

static void ensure_data_dir(void) {
    mkdir("sd:/apps", 0777); mkdir("sd:/apps/backyardcornhole", 0777); mkdir(SAVE_DIR, 0777);
}

static void clear_pose_cal(unsigned ch) {
    debug_channel = ch;
    pose_cal = (HudWiimoteCalibration){0};
    static_tracking = static_pose = false;
}

static uint32_t debug_controls(unsigned ch, const InputSample *p) {
    if (!p->buttons_valid) return 0;
    uint32_t pressed = p->buttons & ~debug_held[ch];
    debug_held[ch] = p->buttons;
    debug_blocked[ch] &= p->buttons;
    if (ch != input.channels[input.active_slot] || !input.debug_visible)
        return debug_blocked[ch];
    if (ch != debug_channel) clear_pose_cal(ch);
    const uint32_t chord = INPUT_ONE | INPUT_TWO;
    if ((p->buttons & chord) == chord) {
        debug_blocked[ch] &= ~INPUT_TWO;
    } else if (pose_cal.active) {
        uint32_t controls = pressed & (INPUT_A | INPUT_TWO | INPUT_LEFT | INPUT_RIGHT);
        debug_blocked[ch] |= controls;
        if (controls & INPUT_TWO) {
            pose_cal.active = false;
            pose_cal.status = HUD_POSE_IDLE;
        } else if (controls & (INPUT_LEFT | INPUT_RIGHT)) {
            unsigned step = controls & INPUT_RIGHT ? 1 : HUD_WIIMOTE_POSES - 1;
            pose_cal.pose = (pose_cal.pose + step) % HUD_WIIMOTE_POSES;
            pose_cal.status = HUD_POSE_IDLE;
        } else if (controls & INPUT_A) {
            pose_cal.status = HUD_POSE_CAPTURING;
            pose_sum = (SwingVector){0};
            pose_reports = 0;
            pose_start_ms = pose_report_ms = p->motion.ms;
        }
    } else if ((pressed & INPUT_A) && !p->motion.b && !swing_armed(&input.detector)) {
        debug_blocked[ch] |= INPUT_A;
        pose_cal.active = true;
    }
    return debug_blocked[ch];
}

static void capture_pose(const SwingSample *p, double length) {
    if (pose_cal.status != HUD_POSE_CAPTURING || !p->accel_valid || p->ms <= pose_start_ms) return;
    if (p->ms - pose_report_ms > SWING_BRIDGE_MS) {
        pose_cal.status = HUD_POSE_INTERRUPTED;
        return;
    }
    if (!isfinite(length) || fabs(length - 1) >= SWING_STATIC_G) {
        pose_cal.status = HUD_POSE_MOVED;
        return;
    }
    pose_report_ms = p->ms;
    if (p->ms - pose_start_ms < POSE_CAPTURE_MS || !pose_reports) {
        pose_sum.x += p->g.x;
        pose_sum.y += p->g.y;
        pose_sum.z += p->g.z;
        ++pose_reports;
    }
    if (p->ms - pose_start_ms >= POSE_CAPTURE_MS) {
        unsigned pose = pose_cal.pose;
        SwingVector mean = {pose_sum.x / pose_reports, pose_sum.y / pose_reports, pose_sum.z / pose_reports};
        if (fabs(sqrt(mean.x * mean.x + mean.y * mean.y + mean.z * mean.z) - 1) >= SWING_STATIC_G) {
            pose_cal.status = HUD_POSE_MOVED;
            return;
        }
        pose_cal.poses[pose] = mean;
        pose_cal.captured[pose] = true;
        pose_cal.status = HUD_POSE_IDLE;
    }
}

static bool buffer_trace(const char *line, size_t size) {
    if (size > sizeof trace_buffer - trace_used) return false;
    memcpy(trace_buffer + trace_used, line, size);
    trace_used += size;
    return true;
}

static bool write_trace_sd(const char *dropped) {
    ensure_data_dir();
    struct stat info;
    bool exists = stat(SWING_TRACE, &info) == 0;
    if (exists && (size_t)info.st_size + trace_used + strlen(dropped) > SWING_TRACE_LIMIT) {
        remove(SWING_TRACE_OLD);
        if (rename(SWING_TRACE, SWING_TRACE_OLD) != 0) return false;
        exists = false;
    }
    FILE *file = fopen(SWING_TRACE, "a");
    if (!file) return false;
    bool ok = true;
    if ((!exists || info.st_size == 0) && trace_buffer[0] != '#') {
        ok = fputs("# continued session\n", file) != EOF && fputs(trace_columns, file) != EOF;
    }
    ok = ok && fwrite(trace_buffer, 1, trace_used, file) == trace_used && fputs(dropped, file) != EOF;
    return fclose(file) == 0 && ok;
}

/* Called outside the WPAD drain. Keep writes off the swing path, even if the
 * overlay closes while B is held. A full buffer drops new rows until idle. */
static void flush_swing_trace(bool force) {
    if (!trace_used || swing_armed(&input.detector) || (!force && trace_used < SWING_TRACE_FLUSH)) return;
    char dropped[64] = "";
    if (trace_dropped) snprintf(dropped, sizeof dropped, "# dropped %u buffered reports\n", trace_dropped);
    if (fat_ok && !trace_sd_failed) trace_sd_failed = !write_trace_sd(dropped);
    if (!fat_ok || trace_sd_failed) {
        const char *line = trace_buffer, *end = trace_buffer + trace_used;
        while (line < end) {
            size_t length = (size_t)((const char *)memchr(line, '\n', (size_t)(end - line)) - line) + 1;
            SYS_Report("BYC-SWING,%.*s", (int)length, line);
            line += length;
        }
        if (trace_dropped) SYS_Report("BYC-SWING,%s", dropped);
    }
    trace_used = trace_dropped = 0;
}

static void on_wiimote_report(void *user, const InputWpadTrace *row) {
    (void)user;
    if (!input.debug_visible) {
        tracing = false;
        return;
    }
    if (row->channel != input.channels[row->player_slot]) return;
    if (row->channel != debug_channel) clear_pose_cal(row->channel);
    const SwingSample *p = &row->sample->motion;
    const Swing *s = row->detector;
    double length = sqrt(p->g.x * p->g.x + p->g.y * p->g.y + p->g.z * p->g.z);
    if (p->accel_valid && fabs(length - 1) < SWING_STATIC_G) {
        if (!static_tracking || p->ms - static_report_ms > SWING_GAP_MS) static_start_ms = p->ms;
        static_tracking = true;
        static_report_ms = p->ms;
        static_pose = p->ms - static_start_ms >= SWING_STATIC_MS;
    } else static_tracking = static_pose = false;
    capture_pose(p, length);

    char line[256];
    if (!tracing) {
        int size = snprintf(line, sizeof line, "# session %s at %llu ms\n%s", SWING_VERSION,
                            (unsigned long long)row->estimated_ms, trace_columns);
        tracing = size > 0 && (size_t)size < sizeof line && buffer_trace(line, (size_t)size);
    }
    int size = snprintf(line, sizeof line, "%s,%llu,%llu,%u,%u,%u,%u,%u,%u,%u,%d,%d,%d,"
             "%+.5f,%+.5f,%+.5f,%.5f,%s,%+.5f,%+.5f,%+.5f,%d,%s\n",
             SWING_VERSION, (unsigned long long)row->estimated_ms, (unsigned long long)row->drain_ms,
             row->channel + 1, row->player_slot + 1, row->event_index, row->dropped,
             (unsigned)row->raw->accel.x, (unsigned)row->raw->accel.y, (unsigned)row->raw->accel.z,
             p->accel_valid, row->sample->buttons_valid, p->clipped,
             p->g.x, p->g.y, p->g.z, length, swing_phase_name(s), s->radial, s->tangent, s->excess,
             p->b, swing_reject_name(s->last_reject));
    if (!tracing || size <= 0 || (size_t)size >= sizeof line || !buffer_trace(line, (size_t)size))
        ++trace_dropped;
}

#ifdef BYC_SOAK
/* Dev-only soak build (make soak): scripted auto-throws through full 2 PLAYER matches, then a
 * 48-bag practice load, logging results and frame CPU time to Dolphin's OSReport. Never shipped. */
static int soak_rounds, soak_matches, soak_wait, soak_stage, soak_practice_throws;
static double soak_cpu_ms, soak_cpu_max; static unsigned soak_frames;
static void soak_drive(Game *g) {
    if (g->ui == UI_STRAP) {
        /* also exercise the wind streaks and, when the SD card has MP3s, the MUSIC player */
        g->save.wind_on = true;
        if (g->music && music_can_toggle(g->music) && !g->music->enabled) music_toggle(g->music);
        game_menu_select(g, MI_OK); return;
    }
    if (soak_stage == 0 && g->ui == UI_TITLE) { game_start(g, false, false, false); SYS_Report("BYC-SOAK: 2 PLAYER match %d\n", soak_matches + 1); return; }
    if (g->ui == UI_SUMMARY) {
        soak_rounds++;
        SYS_Report("BYC-SOAK: round %d %s RED %d BLUE %d (round %d-%d)\n", g->round.inning,
                   g->round.scorer < 0 ? "WASH" : g->round.scorer ? "BLUE" : "RED", g->scores[0], g->scores[1], g->round.lie.pts[0], g->round.lie.pts[1]);
        if (g->phase == PH_OVER) { soak_matches++; SYS_Report("BYC-SOAK: match over, %s wins %d-%d\n", g->winner ? "BLUE" : "RED", g->scores[0], g->scores[1]); }
        game_menu_select(g, MI_NEXT);
        if (soak_rounds >= 20 && g->phase == PH_MENU) { soak_stage = 1; game_start(g, true, false, false); SYS_Report("BYC-SOAK: practice load start\n"); }
        return;
    }
    if (g->phase == PH_AIM && g->ui == UI_NONE && !g->connect_prompt && game_is_human(g, game_thrower(g))) {
        if (++soak_wait < 40) return;
        soak_wait = 0;
        int t = game_thrower(g);
        double mid = g->win->w[t][g->side[t]][g->arc].mid;
        double p = mid + (rng_next(&g->rng) - .5) * .12, aim = (rng_next(&g->rng) - .5) * .05, spin = (rng_next(&g->rng) - .5) * 1.4;
        if (game_throw(g, p, aim, spin) && soak_stage == 1 && ++soak_practice_throws == 48)
            SYS_Report("BYC-SOAK: 48 practice bags thrown (%d retained)\n", g->nbags);
    }
}
#endif

/* ---- saves: save.tmp -> flush/close -> save.dat => save.bak -> save.tmp => save.dat ---- */
static bool read_file(const char *path, SaveData *out) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    uint8_t buf[SAVE_MAX_BYTES + 1];
    size_t n = fread(buf, 1, sizeof buf, f);
    fclose(f);
    return n <= SAVE_MAX_BYTES && save_decode(out, buf, n);
}
static bool load_save(SaveData *out) {
    if (read_file(SAVE_DAT, out)) return true;
    if (read_file(SAVE_BAK, out)) return true;
    save_defaults(out);
    return false;
}
static bool write_save(const SaveData *s) {
    if (!fat_ok) return false;
    uint8_t buf[SAVE_MAX_BYTES];
    size_t n = save_encode(s, buf, sizeof buf);
    if (!n) return false;
    ensure_data_dir();
    FILE *f = fopen(SAVE_TMP, "wb");
    if (!f) return false;
    bool ok = fwrite(buf, 1, n, f) == n;
    ok = fflush(f) == 0 && ok;
    ok = fclose(f) == 0 && ok;
    if (!ok) return false;
    remove(SAVE_BAK);
    rename(SAVE_DAT, SAVE_BAK);          /* may fail on first save: no previous file */
    return rename(SAVE_TMP, SAVE_DAT) == 0;
}

static void rumble_update(int frames_elapsed) {
    for (int ch = 0; ch < 2; ch++) {
        if (rumble_left[ch] <= 0) continue;
        rumble_left[ch] -= frames_elapsed * 16;
        if (rumble_left[ch] <= 0) WPAD_Rumble(ch, 0);
    }
}

/* Physics steps the CPU search may run per frame, from a measured ~4 ms slice (SPEC §2.4). */
static long measure_step_budget(void) {
    Bag b; Wind calm = {0, 0, 0, 0};
    long steps = 0;
    uint64_t t0 = gettime();
    while (steps < 4000) {
        bag_new(&b, 1, SIDE_SLICK); bag_launch(&b, 0, 0.6, 0, 0, ARC_NORMAL, 0);
        for (int i = 0; i < 1920; i++) { bag_step(&b, &calm, 1.0 / 240); steps++; if (bag_is_final(&b)) break; }
    }
    uint64_t us = ticks_to_microsecs(diff_ticks(t0, gettime()));
    if (us < 1) us = 1;
    long budget = (long)(steps * 4000.0 / (double)us);
    SYS_Report("BYC: physics %ld steps in %llu us -> CPU budget %ld steps/frame\n", steps, (unsigned long long)us, budget);
    return budget < 500 ? 500 : budget;
}

int main(void) {
    SYS_STDIO_Report(true);
    gfx_init();
    SYS_SetResetCallback(on_reset);
    SYS_SetPowerCallback(on_power);
    SYS_Report("BYC: Backyard Cornhole Wii " BYC_VERSION " boot; video %s%s, 60 Hz NTSC\n",
               gfx_progressive ? "480p" : "480i", gfx_widescreen ? " 16:9" : " 4:3");

    fat_ok = fatInitDefault();
    SaveData save;
    bool loaded = fat_ok && load_save(&save);
    if (!loaded) save_defaults(&save);
    SYS_Report("BYC: SD %s, save %s\n", fat_ok ? "mounted" : "missing", loaded ? "loaded" : "defaults");

    input_init(&input, on_input, &game);
    int status = input_wpad_init(&wpad, &input, on_wiimote_report, NULL);
    wpad.button_filter = debug_controls;
    WPAD_SetPowerButtonCallback(on_wpad_power);
#ifdef BYC_SOAK
    WPAD_SetIdleTimeout(3600);   /* soak only: nobody presses buttons; release keeps the 5-minute default */
#endif
    SYS_Report("BYC: WPAD init %d (buttons + accel; A or 2 confirms menus)\n", status);

    scene_init();
    game_init(&game, &input, &byc_win, (uint32_t)gettime(), &save, fat_ok);
    game.cpu_step_budget = measure_step_budget();
    /* MUSIC: the player's own MP3s from the SD card (never bundled). */
    MusicBackend audio = music_wii_backend(fat_ok);
    music_init(&music, &audio, game.save.music_on, game.save.music_volume, game.save.music_folder);
    game_set_music(&game, &music);
    SYS_Report("BYC: music folder %s: %d track(s), %s, volume %d%%\n", music.folder, music.count, music.enabled ? "on" : "off", (int)(music.volume * 100 + .5));
    SYS_Report("BYC: WIN NORMAL/SLICK lo %.4f hi %.4f mid %.4f\n", byc_win.w[0][0][1].lo, byc_win.w[0][0][1].hi, byc_win.w[0][0][1].mid);

    uint64_t last = now_ms();
    unsigned frame = 0;
    while (!power_off && !reset_pressed && !game.quit_requested) {
        uint64_t now = now_ms();
        double dt = (double)(now - last) / 1000.0;
        if (dt <= 0 || dt > 0.25) dt = 1.0 / 60;
        last = now;

        bool debug_was_visible = input.debug_visible;
        input_wpad_poll(&wpad, now);
        for (unsigned ch = 0; ch < 2; ++ch)
            if (!input.connected[ch]) debug_held[ch] = debug_blocked[ch] = 0;
        if (pose_cal.status == HUD_POSE_CAPTURING && now - pose_report_ms > SWING_BRIDGE_MS)
            pose_cal.status = HUD_POSE_INTERRUPTED;
        if (!input.debug_visible && debug_was_visible) {
            pose_cal.active = false;
            pose_cal.status = HUD_POSE_IDLE;
            static_tracking = static_pose = false;
            tracing = false;
        }
        if (input.debug_visible != debug_was_visible)
            SYS_Report("BYC: wiimote debug %s\n", input.debug_visible ? "on" : "off");
        for (int i = 0; i < queued; i++) { log_event(&queue[i]); game_input(&game, &queue[i]); }
        queued = 0;
#ifdef BYC_SOAK
        uint64_t cpu0 = gettime();
        soak_drive(&game);
#endif
        game_update(&game, dt, now);
        for (int i = 0; i < queued; i++) game_input(&game, &queue[i]);   /* events from state changes */
        queued = 0;
        unsigned active_ch = input.channels[input.active_slot];
        if (active_ch != debug_channel || !input.connected[active_ch]) clear_pose_cal(active_ch);
        flush_swing_trace(!input.debug_visible);

        Fx fx;
        while (game_pop_fx(&game, &fx)) {
            if (fx.kind == FX_RUMBLE) {
                unsigned ch = fx.channel < 2 ? fx.channel : 0;
                if (input.connected[ch]) { WPAD_Rumble((s32)ch, 1); rumble_left[ch] = fx.ms; }
            } else scene_fx(&fx);
        }
        rumble_update(1);
        scene_update(&game, dt);

        if (game.save_dirty) {
            game.save_dirty = false;
            if (!write_save(&game.save)) game.save_ok = false;
        }

        float dip = 0;
        if (game.phase == PH_AIM && game.ui == UI_NONE && swing_armed(&input.detector)) dip = 1;

        gfx_begin_frame();
        scene_draw(&game, dip);
        if (hud_board_view_visible(&game)) {
            int x, y, w, h; hud_board_view_rect(&x, &y, &w, &h);
            int x0 = (int)lroundf(gfx_ui_to_efb((float)x)), x1 = (int)lroundf(gfx_ui_to_efb((float)(x + w)));
            scene_draw_board_view(&game, x0, y, x1 - x0, h);
        }
        hud_draw(&game, &input);
        if (input.debug_visible)
            hud_draw_wiimote_debug(&input, &wpad.latest[active_ch], &pose_cal, static_pose);
#ifdef BYC_SOAK
        double cpu = ticks_to_microsecs(diff_ticks(cpu0, gettime())) / 1000.0;
        soak_cpu_ms += cpu; if (cpu > soak_cpu_max) soak_cpu_max = cpu;
        if (++soak_frames % 600 == 0) {
            SYS_Report("BYC-SOAK: frames %u, CPU avg %.2f ms max %.2f ms, bags %d, phase %d\n", soak_frames, soak_cpu_ms / 600, soak_cpu_max, game.nbags, game.phase);
            soak_cpu_ms = 0; soak_cpu_max = 0;
        }
        if (soak_stage == 1 && soak_practice_throws >= 48 && game.phase == PH_AIM) {
            SYS_Report("BYC-SOAK: done: %d matches, %d rounds, 48-bag practice\n", soak_matches, soak_rounds);
            game.quit_requested = true;
        }
#endif
        gfx_end_frame();

        static bool last_conn[2];
        for (int ch = 0; ch < 2; ch++) if (input.connected[ch] != last_conn[ch]) {
            last_conn[ch] = input.connected[ch];
            SYS_Report("BYC: remote %d %s (WPAD status %d)\n", ch + 1, last_conn[ch] ? "connected" : "disconnected", wpad.status[ch]);
        }
        static int last_track = -2; static bool last_music;
        int track_now = music.started ? music.index : -1;
        if (track_now != last_track || music.enabled != last_music) {
            last_track = track_now; last_music = music.enabled;
            SYS_Report("BYC: music %s: %s\n", music.enabled ? "on" : "off", music_now(&music));
        }
        static int last_ui = -1, last_phase = -1;
        if ((int)game.ui != last_ui || (int)game.phase != last_phase) {
            last_ui = game.ui; last_phase = game.phase;
            SYS_Report("BYC: screen %d phase %d (remote 1 %s)\n", last_ui, last_phase, input.connected[0] ? "connected" : "missing");
        }
        if (game.msg_on && game.msg_big[0]) {
            static char last_msg[24];
            if (strcmp(last_msg, game.msg_big) != 0) {
                snprintf(last_msg, sizeof last_msg, "%s", game.msg_big);
                SYS_Report("BYC: callout '%s' '%s' score %d-%d practice in=%d on=%d throws=%d\n",
                           game.msg_big, game.msg_small, game.scores[0], game.scores[1],
                           game.practice_in, game.practice_on, game.practice_throws);
            }
        }
        {
            static uint64_t fps_anchor;
            if (fps_anchor == 0) fps_anchor = now;
            if (++frame % 120 == 0) {
                uint64_t elapsed = now - fps_anchor;
                double fps = elapsed ? (120000.0 / (double)elapsed) : 0;
                SYS_Report("BYC: %u frames rendered; ui %d phase %d; fps %.1f\n",
                           frame, game.ui, game.phase, fps);
                fps_anchor = now;
            }
        }
    }
    swing_cancel(&input.detector, true);
    flush_swing_trace(true);
    for (int ch = 0; ch < 2; ch++) WPAD_Rumble(ch, 0);
    music_shutdown(&music);
    music_wii_shutdown();
    SYS_Report("BYC: exit (%s)\n", power_off ? "power" : reset_pressed ? "reset" : "quit");
    WPAD_Shutdown();
    if (fat_ok) fatUnmount("sd");
    if (power_off) SYS_ResetSystem(SYS_POWEROFF, 0, 0);
    return 0;   /* returns to the Homebrew Channel loader */
}
