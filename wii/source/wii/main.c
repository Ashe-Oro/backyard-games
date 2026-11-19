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

static Input input;
static InputWpad wpad;
static Game game;
static Music music;
static InputEvent queue[64];
static int queued;
static volatile bool power_off, reset_pressed;
static bool fat_ok;
static int rumble_left[2];

static void on_input(void *user, const InputEvent *e) {
    (void)user;
    if (queued < 64) queue[queued++] = *e;   /* applied after input_wpad_poll returns */
}
/* Debug UART only (Dolphin OSReport / USB Gecko); never on screen. Helps P1 trace sessions. */
static void log_event(const InputEvent *e) {
    if (e->action == INPUT_THROW)
        SYS_Report("BYC: throw slot %u power %.3f speed %.2f aim %+.3f spin %+.2f roll %+.1f release %d%s\n", e->slot,
                   e->toss.power, e->toss.speed, e->toss.aim, e->toss.spin, e->toss.roll, (int)e->toss.release, e->toss.clipped ? " clipped" : "");
    else if (e->action == INPUT_HOLD_STILL) SYS_Report("BYC: hold still (B pressed while moving)\n");
    else if (e->action == INPUT_CONNECT_REMOTE) SYS_Report("BYC: connect remote %d\n", (int)e->value);
    else if (e->action == INPUT_CAL_DONE) SYS_Report("BYC: SET POWER done slot %u\n", e->slot);
    else SYS_Report("BYC: input action %d value %g\n", (int)e->action, e->value);
}
static void on_reset(u32 irq, void *ctx) { (void)irq; (void)ctx; reset_pressed = true; }
static void on_power(void) { power_off = true; }
static void on_wpad_power(s32 chan) { (void)chan; power_off = true; }

static uint64_t now_ms(void) { return ticks_to_millisecs(gettime()); }

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
    mkdir("sd:/apps", 0777); mkdir("sd:/apps/backyardcornhole", 0777); mkdir(SAVE_DIR, 0777);
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
    int status = input_wpad_init(&wpad, &input, NULL, NULL);
    WPAD_SetPowerButtonCallback(on_wpad_power);
#ifdef BYC_SOAK
    WPAD_SetIdleTimeout(3600);   /* soak only: nobody presses buttons; release keeps the 5-minute default */
#endif
    if (gfx_mode) input_wpad_set_vres(&wpad, gfx_mode->fbWidth, gfx_mode->xfbHeight);
    SYS_Report("BYC: WPAD init %d (buttons + accel + optional IR; A or 2 confirms menus)\n", status);

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

        input_wpad_poll(&wpad, now);
        for (int i = 0; i < queued; i++) { log_event(&queue[i]); game_input(&game, &queue[i]); }
        queued = 0;
#ifdef BYC_SOAK
        uint64_t cpu0 = gettime();
        soak_drive(&game);
#endif
        game_update(&game, dt, now);
        for (int i = 0; i < queued; i++) game_input(&game, &queue[i]);   /* events from state changes */
        queued = 0;

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
        SwingPhase sp = input.detector.phase;
        if (game.phase == PH_AIM && game.ui == UI_NONE && (sp == SWING_GRIP || sp == SWING_PENDING)) dip = 1;

        gfx_begin_frame();
        scene_draw(&game, dip);
        if (hud_board_view_visible(&game)) {
            int x, y, w, h; hud_board_view_rect(&x, &y, &w, &h);
            scene_draw_board_view(&game, x, y, w, h);
        }
        hud_draw(&game, &input);
        {
            InputPointer ptr;
            for (unsigned ch = 0; ch < 2; ch++)
                if (input_wpad_pointer(&wpad, ch, &ptr)) { hud_draw_pointer(ptr.x, ptr.y); break; }
        }
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
    for (int ch = 0; ch < 2; ch++) WPAD_Rumble(ch, 0);
    music_shutdown(&music);
    music_wii_shutdown();
    SYS_Report("BYC: exit (%s)\n", power_off ? "power" : reset_pressed ? "reset" : "quit");
    WPAD_Shutdown();
    if (fat_ok) fatUnmount("sd");
    if (power_off) SYS_ResetSystem(SYS_POWEROFF, 0, 0);
    return 0;   /* returns to the Homebrew Channel loader */
}
