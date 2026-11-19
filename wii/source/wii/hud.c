/* HUD + menus at 640x480 (SPEC §2.6). Palette: teal #173f3d, cream #fff1cf, gold #ffe16b,
 * red #f1755f, blue #8fc6ee, outline #172c32. Text uses the 8x16 font (16 px tall at 1x). */
#include "hud.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define TEAL 0x173f3d
#define CREAM 0xfff1cf
#define GOLD 0xffe16b
#define RED 0xf1755f
#define BLUE 0x8fc6ee
#define INK 0x172c32
#define DOT "\xFA"

static Rgba power_color(double p) {
    static const unsigned stops[5] = {0x8fc6ee, 0xa7efb1, 0xffe16b, 0xf6a35c, 0xf1755f};
    p = fmax(0, fmin(1, p));
    double pos = p * 4; int i = (int)floor(pos); if (i > 3) i = 3;
    double f = pos - i;
    Rgba a = rgb(stops[i]), b = rgb(stops[i + 1]);
    return (Rgba){(u8)(a.r + (b.r - a.r) * f), (u8)(a.g + (b.g - a.g) * f), (u8)(a.b + (b.b - a.b) * f), 255};
}
static void card(float x, float y, float w, float h) {
    gfx_rect(x + 5, y + 5, w, h, rgba(INK, 0x66));
    gfx_rect(x, y, w, h, rgba(TEAL, 0xf2));
    gfx_frame(x, y, w, h, 3, rgb(INK));
}

static void scoreboard(const Game *g) {
    const float w = 360, x = 320 - w / 2, y = 14, h = 62;
    card(x, y, w, h);
    for (int t = 0; t < 2; t++) {
        float cx = t ? x + w - 62 : x + 62;
        bool inactive = g->practice && t == 1;
        bool current = !game_menu_screen(g) && g->phase == PH_AIM && game_thrower(g) == t;
        Rgba col = rgba(t ? BLUE : RED, inactive ? 0x55 : 0xff);
        if (current) gfx_frame(cx - 58, y + 4, 116, h - 8, 2, rgb(GOLD));
        const char *name = t == 0 ? "RED" : g->career_match ? byc_roster[g->fight < 8 ? g->fight : 7].name : "BLUE";
        char nm[16]; snprintf(nm, sizeof nm, "%.13s", name);
        gfx_text_center(cx, y + 6, 1, col, nm);
        char buf[16];
        int score = g->practice ? g->live.pts[t] : g->scores[t];
        snprintf(buf, sizeof buf, "%d", score);
        if (g->practice) {
            gfx_text_center(cx, y + 22, .5f * 2, rgba(CREAM, inactive ? 0x55 : 0xff), "POINTS");
            gfx_text_center(cx, y + 38, 1, rgba(CREAM, inactive ? 0x55 : 0xff), buf);
        } else {
            gfx_text(cx - 50, y + 22, 1, rgb(CREAM), "TOTAL");
            gfx_text(cx - 46 + (score >= 10 ? 0 : 8), y + 38, 1, rgb(CREAM), buf);
            gfx_text(cx + 8, y + 22, 1, rgba(CREAM, 0xb0), "RND");
            snprintf(buf, sizeof buf, "%d", g->live.pts[t]);
            gfx_text(cx + 16, y + 38, 1, rgba(CREAM, 0xb0), buf);
            int thrown = 0;
            for (int i = 0; i < g->nbags; i++) thrown += g->bags[i].team == t;
            for (int i = 0; i < 4; i++) gfx_rect(cx - 22 + i * 12, y + h - 9, 8, 4, i < thrown ? rgba(CREAM, 0x40) : col);
        }
    }
    char mid[32];
    if (g->practice) {
        gfx_text_center(320, y + 6, 1, rgb(GOLD), "PRACTICE");
        int n = g->practice_throws;
        snprintf(mid, sizeof mid, "%d IN", g->practice_in); gfx_text_center(320, y + 22, 1, rgb(CREAM), mid);
        snprintf(mid, sizeof mid, "%d ON", g->practice_on); gfx_text_center(320, y + 38, 1, rgb(CREAM), mid);
        snprintf(mid, sizeof mid, "%d%% HIT " DOT " %d THROWS", n ? (int)lround((g->practice_in + g->practice_on) * 100.0 / n) : 0, n);
        card(320 - gfx_text_width(mid, 1) / 2 - 8, y + h + 4, gfx_text_width(mid, 1) + 16, 22);
        gfx_text_center(320, y + h + 7, 1, rgb(CREAM), mid);
    } else {
        snprintf(mid, sizeof mid, "ROUND %d", g->inning);
        gfx_text_center(320, y + 6, 1, rgb(CREAM), mid);
        gfx_text_center(320, y + 22, 1, rgb(GOLD), g->phase == PH_OVER ? "FINAL" : "TO 21");
        if (g->phase != PH_MENU && g->phase != PH_INTRO && g->phase != PH_CHAMPION) {
            int d = g->live.pts[0] - g->live.pts[1];
            if (!d) snprintf(mid, sizeof mid, "EVEN"); else snprintf(mid, sizeof mid, "%s +%d", d > 0 ? "RED" : "BLUE", d > 0 ? d : -d);
            gfx_text_center(320, y + 40, 1, d > 0 ? rgb(RED) : d < 0 ? rgb(BLUE) : rgb(CREAM), mid);
        }
    }
}

static void wind_panel(const Game *g) {
    if (g->wind.mph <= 0) return;
    const float w = 84, h = 76, x = 640 - 28 - w, y = 168;
    card(x, y, w, h);
    gfx_text_center(x + w / 2, y + 6, 1, rgb(GOLD), "WIND");
    /* arrow points where the wind blows: up = away from the thrower; rotate by angle */
    float cx = x + w / 2, cy = y + 38, a = (float)g->wind.angle, s = sinf(a), c = cosf(a);
#define RX(px, py) (cx + (px) * c - (py) * s)
#define RY(px, py) (cy + (px) * s + (py) * c)
    gfx_tri2(RX(0, -14), RY(0, -14), RX(10, -2), RY(10, -2), RX(-10, -2), RY(-10, -2), rgb(GOLD));
    gfx_tri2(RX(-3, -2), RY(-3, -2), RX(3, -2), RY(3, -2), RX(3, 13), RY(3, 13), rgb(GOLD));
    gfx_tri2(RX(-3, -2), RY(-3, -2), RX(3, 13), RY(3, 13), RX(-3, 13), RY(-3, 13), rgb(GOLD));
#undef RX
#undef RY
    char buf[16]; snprintf(buf, sizeof buf, "%d MPH", (int)g->wind.mph);
    gfx_text_center(x + w / 2, y + 56, 1, rgb(GOLD), buf);
}

static void turn_banner(const Game *g) {
    if (!(g->phase == PH_AIM || g->phase == PH_CPU) || game_menu_screen(g)) return;
    int t = game_thrower(g);
    char buf[32];
    if (g->vs_cpu) snprintf(buf, sizeof buf, "%s", t == 0 ? "YOUR THROW" : "CPU'S THROW");
    else snprintf(buf, sizeof buf, "%s'S THROW", t ? "BLUE" : "RED");
    float cx = hud_board_view_visible(g) ? 396 : 320;   /* arcade.css #turn {left:200px; top:112px} */
    gfx_text_center_shadow(cx, 106, 2, rgb(t ? BLUE : RED), buf);
    if (g->pass_and_play && g->phase == PH_AIM) gfx_text_center_shadow(cx, 140, 1, rgb(CREAM), "PASS THE REMOTE");
}

static void callout(const Game *g) {
    if (!g->msg_on) return;
    /* arcade.css #msg {left:200px; right:16px}: centred right of the board-view inset */
    float s = strlen(g->msg_big) > 9 ? 2 : 3, cx = hud_board_view_visible(g) ? 396 : 320;
    gfx_text_center_shadow(cx, 166, s, rgb(0xffffff), g->msg_big);
    if (g->msg_small[0]) gfx_text_center_shadow(cx, 166 + 16 * s + 8, 1, rgb(GOLD), g->msg_small);
}

/* The one combined direction + power arrow (arcade 1.0.7 / COMBINED_ARROW_NOTES.md). Geometry comes
 * from core/arrow.c (host-tested): origin at the bag, direction = the launch bearing projected
 * through the main camera, coloured shaft length = power on a faint full-power track. There is no
 * separate power meter or LAST SHOT gauge. */
static void line2(float x0, float y0, float x1, float y1, float half, float nx, float ny, Rgba c) {
    gfx_tri2(x0 + nx * half, y0 + ny * half, x1 + nx * half, y1 + ny * half, x1 - nx * half, y1 - ny * half, c);
    gfx_tri2(x0 + nx * half, y0 + ny * half, x1 - nx * half, y1 - ny * half, x0 - nx * half, y0 - ny * half, c);
}
static void shot_arrow(const Game *g) {
    if (!g->arrow_on || game_menu_screen(g) || g->connect_prompt) return;
    ArrowShape s;
    if (!arrow_shape(g->arrow_x, g->arrow_yaw, g->arrow_power, gfx_aspect, &s)) return;
    float a = (float)arrow_alpha(g->arrow_age_ms, g->arrow_live);
    if (a <= 0) return;
    float ox = (float)s.ox, oy = (float)s.oy, dx = (float)s.dx, dy = (float)s.dy, nx = -dy, ny = dx;
    const float head = 8;
    float len = (float)s.length, shaft = fmaxf(0, len - head);
    Rgba c = power_color(g->arrow_power), ink = rgba(INK, (u8)(255 * a)), track = rgba(CREAM, (u8)(0x58 * a));
    c.a = (u8)(255 * a);
    /* faint full-power track + end mark */
    line2(ox, oy, (float)s.ex, (float)s.ey, 1, nx, ny, track);
    line2((float)s.ex - nx * 4, (float)s.ey - ny * 4, (float)s.ex + nx * 4, (float)s.ey + ny * 4, 1, dx, dy, track);
    /* outlined shaft filled to the current power, then the head */
    float bx = ox + dx * shaft, by = oy + dy * shaft, tx = (float)s.tx, ty = (float)s.ty;
    line2(ox - dx, oy - dy, bx, by, 3, nx, ny, ink);
    gfx_tri2(tx + dx * 2, ty + dy * 2, bx + nx * 7 - dx, by + ny * 7 - dy, bx - nx * 7 - dx, by - ny * 7 - dy, ink);
    line2(ox, oy, bx, by, 1.5f, nx, ny, c);
    gfx_tri2(tx, ty, bx + nx * 5, by + ny * 5, bx - nx * 5, by - ny * 5, c);
}

static void board_view_frame(void) {
    const int x = 28, y = 92, w = 132, h = 150;
    gfx_rect(x - 3 + 5, y - 22 + 5, w + 6, h + 25, rgba(INK, 0x66));
    gfx_frame(x - 3, y - 22, w + 6, h + 25, 3, rgb(INK));
    gfx_rect(x, y - 19, w, 19, rgb(TEAL));
    gfx_text_center(x + w / 2, y - 18, 1, rgb(GOLD), "BOARD " DOT " 27 FT");
}
void hud_board_view_rect(int *x, int *y, int *w, int *h) { *x = 28; *y = 92; *w = 132; *h = 150; }
bool hud_board_view_visible(const Game *g) {
    return g->phase != PH_MENU && g->phase != PH_INTRO && g->phase != PH_CHAMPION &&
           !(g->ui == UI_TITLE || g->ui == UI_SETTINGS || g->ui == UI_SETPOWER || g->ui == UI_CAREER_MAP || g->ui == UI_STRAP);
}

static void help_legend(const Game *g, const Input *in) {
    if (game_menu_screen(g) || g->phase == PH_MENU) return;
    int t = game_thrower(g);
    static const char *const heights[3] = {"SLIDER", "NORMAL", "KOBE"};
    char status[48];
    const char *state = "-";
    if (in && g->phase == PH_AIM && game_is_human(g, t)) {
        SwingPhase p = in->detector.phase;
        state = p == SWING_SETTLING ? DOT : p == SWING_READY ? "\x18" : "\x18\x1a";
    }
    snprintf(status, sizeof status, "%s %s " DOT " %s", state, heights[g->arc], g->side[t] == SIDE_STICKY ? "STICKY" : "SLICK");
    float y = 480 - 40;
    if (g->help_shown) {
        /* input_help_text split into TV-safe lines at its ' | ' separators */
        char lines[10][48]; int n = 0;
        const char *s = input_help_text;
        while (*s && n < 10) {
            const char *e = strstr(s, " | ");
            size_t len = e ? (size_t)(e - s) : strlen(s);
            if (len > 47) len = 47;
            memcpy(lines[n], s, len); lines[n][len] = 0; n++;
            if (!e) break;
            s = e + 3;
        }
        float w = 0;
        for (int i = 0; i < n; i++) w = fmaxf(w, gfx_text_width(lines[i], 1));
        float h = n * 16 + 8, x = 28, top = y - h - 6;
        gfx_rect(x - 6, top, w + 12, h, rgba(TEAL, 0xd8));
        for (int i = 0; i < n; i++) gfx_text(x, top + 4 + i * 16, 1, rgb(CREAM), lines[i]);
    }
    gfx_rect(22, y - 2, gfx_text_width(status, 1) + 12 + 56, 22, rgba(TEAL, 0xe8));
    gfx_text(28, y + 1, 1, rgb(GOLD), status);
    gfx_text(28 + gfx_text_width(status, 1) + 8, y + 1, 1, rgba(CREAM, 0xb0), g->help_shown ? "1:HIDE" : "1:HELP");
    if (in && g->phase == PH_AIM && game_is_human(g, t) && fabs(in->slots[t].aim) > 1e-9) {
        char aim[24]; snprintf(aim, sizeof aim, "AIM: %+d", (int)lround(in->slots[t].aim / SWING_AIM_NUDGE));
        gfx_text_shadow(28, y - 22 - (g->help_shown ? 0 : 0), 1, rgb(GOLD), aim);
    }
}

static void menu_list(const Game *g, float cx, float y, float w) {
    for (int i = 0; i < g->menu.count; i++) {
        const MenuItem *it = &g->menu.items[i];
        bool sel = i == g->menu.sel;
        float iy = y + i * 24;
        if (sel) gfx_rect(cx - w / 2, iy - 3, w, 22, rgb(GOLD));
        Rgba c = sel ? rgba(INK, it->disabled ? 0x80 : 0xff) : rgba(CREAM, it->disabled ? 0x60 : 0xff);
        if (it->value[0]) {
            gfx_text(cx - w / 2 + 10, iy, 1, c, it->label);
            gfx_text(cx + w / 2 - 10 - gfx_text_width(it->value, 1), iy, 1, sel ? rgb(INK) : rgb(GOLD), it->value);
        } else gfx_text_center(cx, iy, 1, c, it->label);
    }
}

static void wrap_center(float cx, float y, float maxw, Rgba c, const char *text) {
    char line[64]; size_t maxc = (size_t)(maxw / 8); const char *s = text; float yy = y;
    while (*s) {
        size_t n = strlen(s);
        if (n > maxc) { n = maxc; while (n > 0 && s[n] != ' ') n--; if (!n) n = maxc; }
        if (n > 63) n = 63;
        memcpy(line, s, n); line[n] = 0;
        gfx_text_center(cx, yy, 1, c, line);
        yy += 18; s += n; while (*s == ' ') s++;
    }
}

static void panel(const Game *g) {
    char buf[96];
    float w = 420, x = 320 - w / 2;
    const Rival *o = &byc_roster[g->fight < 8 ? g->fight : 7];
    Tex *portrait = NULL;
    switch (g->ui) {
    case UI_STRAP: {
        float h = 150, y = 150; card(x, y, w, h);
        gfx_text_center(320, y + 24, 3, rgb(GOLD), "STRAP ON!");
        gfx_text_center(320, y + 82, 1, rgb(CREAM), "HOLD B " DOT " SWING " DOT " LET GO");
        menu_list(g, 320, y + 114, 120);
        break;
    }
    case UI_TITLE: {
        float h = 300, y = 96; card(x, y, w, h);
        gfx_text_center(320, y + 14, 1, rgb(GOLD), "WII " DOT " " BYC_VERSION);
        gfx_text_center_shadow(320, y + 38, 3, rgb(CREAM), "BACKYARD");
        gfx_text_center_shadow(320, y + 88, 3, rgb(GOLD), "CORNHOLE");
        menu_list(g, 320, y + 158, 220);
        if (!g->save_ok) gfx_text_center(320, y + h - 22, 1, rgba(CREAM, 0xb0), "Progress won't save on this device.");
        break;
    }
    case UI_PAUSE: {
        float h = 64 + g->menu.count * 24, y = 240 - h / 2; card(x, y, w, h);
        gfx_text_center(320, y + 12, 2, rgb(GOLD), "PAUSED");
        menu_list(g, 320, y + 52, 260);
        gfx_text(x + w - 8 - gfx_text_width("v" BYC_VERSION, 1), y + h + 6, 1, rgba(CREAM, 0xc0), "v" BYC_VERSION);
        break;
    }
    case UI_SETTINGS: {
        float h = 60 + g->menu.count * 24, y = 240 - h / 2; card(x, y, w, h);
        gfx_text_center(320, y + 10, 2, rgb(GOLD), "SETTINGS");
        menu_list(g, 320, y + 48, 380);
        break;
    }
    case UI_SETPOWER: {
        float h = 170, y = 150; card(x, y, w, h);
        gfx_text_center(320, y + 12, 2, rgb(GOLD), g->setpower_slot ? "BLUE SET POWER" : "RED SET POWER");
        gfx_text_center(320, y + 54, 2, rgb(CREAM), "TOSS EASY x3");
        for (int i = 0; i < SWING_CAL_TOSSES; i++) gfx_rect(320 - 40 + i * 30, y + 92, 20, 12, i < g->setpower_count ? rgb(GOLD) : rgba(CREAM, 0x40));
        if (g->setpower_note[0]) gfx_text_center(320, y + 114, 1, rgb(strcmp(g->setpower_note, "POWER SET!") ? RED : GOLD), g->setpower_note);
        gfx_text_center(320, y + 140, 1, rgba(CREAM, 0xc0), "HOME: DONE");
        break;
    }
    case UI_CAREER_MAP: {
        float h = 330, y = 74; card(x, y, w, h);
        gfx_text_center(320, y + 10, 1, rgb(GOLD), "ROAD TO CORNHOLE KING");
        for (int i = 0; i < 8; i++) {
            const char *mark = i < g->fight ? "\xFB" : i == g->fight ? "\x10" : DOT;
            snprintf(buf, sizeof buf, "%s %s " DOT " %s", mark, byc_roster[i].name, byc_circuits[byc_roster[i].circuit]);
            Rgba c = i < g->fight ? rgba(CREAM, 0x90) : i == g->fight ? rgb(GOLD) : rgb(CREAM);
            gfx_text(x + 24, y + 34 + i * 20, 1, c, buf);
        }
        snprintf(buf, sizeof buf, "%d WINS " DOT " %d/8 CLEARED", g->wins, g->fight);
        gfx_text_center(320, y + 200, 1, rgb(CREAM), buf);
        menu_list(g, 320, y + 228, 220);
        if (!g->save_ok) gfx_text_center(320, y + h - 22, 1, rgba(CREAM, 0xb0), "Progress won't save on this device.");
        break;
    }
    case UI_INTRO: {
        float h = 330, y = 74; card(x, y, w, h);
        snprintf(buf, sizeof buf, "%s " DOT " MATCH %d/8", byc_circuits[o->circuit], g->fight + 1);
        gfx_text_center(320, y + 10, 1, rgb(GOLD), buf);
        snprintf(buf, sizeof buf, "NEXT UP: %s", o->name);
        gfx_text_center(320, y + 32, 1, rgb(CREAM), buf);
        scene_portrait(o->level, &portrait);
        tex_bind(portrait); {
            float px = 320 - 40, py = y + 56, s = 80;
            GX_Begin(GX_QUADS, GX_VTXFMT1, 4);
            GX_Position3f32(px, py, -1); GX_Color4u8(255, 255, 255, 255); GX_TexCoord2f32(0, 0);
            GX_Position3f32(px + s, py, -1); GX_Color4u8(255, 255, 255, 255); GX_TexCoord2f32(1, 0);
            GX_Position3f32(px + s, py + s, -1); GX_Color4u8(255, 255, 255, 255); GX_TexCoord2f32(1, 1);
            GX_Position3f32(px, py + s, -1); GX_Color4u8(255, 255, 255, 255); GX_TexCoord2f32(0, 1);
            GX_End();
        }
        snprintf(buf, sizeof buf, "\"%s\"", o->nick); gfx_text_center(320, y + 146, 1, rgb(CREAM), buf);
        wrap_center(320, y + 166, w - 30, rgb(CREAM), o->taunt);
        const char *style = o->style; char up[16]; size_t i = 0; for (; style[i] && i < 15; i++) up[i] = (char)(style[i] >= 'a' && style[i] <= 'z' ? style[i] - 32 : style[i]); up[i] = 0;
        snprintf(buf, sizeof buf, "%s " DOT " %s", up, o->circuit == 0 ? "CALM AIR" : o->circuit == 1 ? "LIGHT WIND" : "STRONG WIND");
        gfx_text_center(320, y + 206, 1, rgb(GOLD), buf);
        menu_list(g, 320, y + 250, 220);
        break;
    }
    case UI_CHAMPION: {
        float h = 300, y = 90; card(x, y, w, h);
        gfx_text_center(320, y + 12, 1, rgb(GOLD), "ALL FOUR CIRCUITS CONQUERED");
        gfx_text_center_shadow(320, y + 40, 3, rgb(GOLD), "CORNHOLE KING!");
        scene_portrait(7, &portrait); tex_bind(portrait); {
            float px = 320 - 40, py = y + 100, s = 80;
            GX_Begin(GX_QUADS, GX_VTXFMT1, 4);
            GX_Position3f32(px, py, -1); GX_Color4u8(255, 255, 255, 255); GX_TexCoord2f32(0, 0);
            GX_Position3f32(px + s, py, -1); GX_Color4u8(255, 255, 255, 255); GX_TexCoord2f32(1, 0);
            GX_Position3f32(px + s, py + s, -1); GX_Color4u8(255, 255, 255, 255); GX_TexCoord2f32(1, 1);
            GX_Position3f32(px, py + s, -1); GX_Color4u8(255, 255, 255, 255); GX_TexCoord2f32(0, 1);
            GX_End();
        }
        gfx_text_center(320, y + 190, 1, rgb(CREAM), "All eight rivals defeated.");
        menu_list(g, 320, y + 224, 220);
        break;
    }
    case UI_SUMMARY: case UI_CAREER_RESULT: {
        const RoundResult *r = &g->round;
        float h = 250, y = 110; card(x, y, w, h);
        if (g->ui == UI_CAREER_RESULT) {
            const Rival *rv = &byc_roster[(g->winner == 0 ? g->fight - 1 : g->fight) < 8 ? (g->winner == 0 ? g->fight - 1 : g->fight) : 7];
            gfx_text_center(320, y + 12, 1, rgb(GOLD), g->winner == 0 ? "CIRCUIT VICTORY" : "RIVAL WINS");
            snprintf(buf, sizeof buf, "%s %s", g->winner == 0 ? "You beat" : "You lost to", rv->name);
            gfx_text_center(320, y + 44, 1, rgb(CREAM), buf);
            snprintf(buf, sizeof buf, "%d - %d", g->scores[0], g->scores[1]);
            gfx_text_center(320, y + 68, 2, rgb(CREAM), buf);
            gfx_text_center(320, y + 110, 1, rgb(CREAM), g->winner == 0 ? "Next rival unlocked!" : "Try again.");
            if (!g->save_ok) gfx_text_center(320, y + 132, 1, rgba(CREAM, 0xb0), "Progress won't save on this device.");
            menu_list(g, 320, y + 170, 220);
            break;
        }
        if (g->phase == PH_OVER) {
            gfx_text_center(320, y + 10, 1, rgb(GOLD), "FINAL");
            if (g->vs_cpu) snprintf(buf, sizeof buf, "%s", g->winner == 0 ? "YOU WIN!" : "CPU WINS");
            else snprintf(buf, sizeof buf, "%s WINS!", g->winner ? "BLUE" : "RED");
            gfx_text_center_shadow(320, y + 30, 3, rgb(g->winner ? BLUE : RED), buf);
        } else {
            snprintf(buf, sizeof buf, "ROUND %d", r->inning); gfx_text_center(320, y + 10, 1, rgb(GOLD), buf);
            if (r->scorer < 0) snprintf(buf, sizeof buf, "WASH"); else snprintf(buf, sizeof buf, "%s +%d", r->scorer ? "BLUE" : "RED", r->net);
            gfx_text_center_shadow(320, y + 30, 3, r->scorer < 0 ? rgb(CREAM) : rgb(r->scorer ? BLUE : RED), buf);
        }
        for (int t = 0; t < 2; t++) {
            float cx = t ? 320 + 110 : 320 - 110;
            gfx_text_center(cx, y + 90, 1, rgb(t ? BLUE : RED), t ? "BLUE" : "RED");
            snprintf(buf, sizeof buf, "%d", r->lie.pts[t]); gfx_text_center(cx, y + 108, 2, rgb(CREAM), buf);
            snprintf(buf, sizeof buf, "%d IN " DOT " %d ON", r->lie.in_c[t], r->lie.on_c[t]); gfx_text_center(cx, y + 142, 1, rgb(CREAM), buf);
        }
        gfx_text_center(320, y + 112, 1, rgb(CREAM), "vs");
        if (r->scorer < 0) snprintf(buf, sizeof buf, "TIED ROUND " DOT " NO POINTS");
        else snprintf(buf, sizeof buf, "%d - %d = %d (CANCELLATION)", r->lie.pts[r->scorer], r->lie.pts[1 - r->scorer], r->net);
        gfx_text_center(320, y + 166, 1, rgb(CREAM), buf);
        snprintf(buf, sizeof buf, "MATCH: RED %d " DOT " BLUE %d", g->scores[0], g->scores[1]);
        gfx_text_center(320, y + 186, 1, rgb(CREAM), buf);
        if (g->phase != PH_OVER) { snprintf(buf, sizeof buf, "%s THROWS FIRST", g->first ? "BLUE" : "RED"); gfx_text_center(320, y + 204, 1, rgb(GOLD), buf); }
        menu_list(g, 320, y + 224, 200);
        break;
    }
    case UI_CONFIRM_QUIT: case UI_CONFIRM_NEW_CAREER: {
        float h = 130, y = 175; card(x, y, w, h);
        gfx_text_center(320, y + 16, 2, rgb(GOLD), g->ui == UI_CONFIRM_QUIT ? "QUIT?" : "NEW CAREER?");
        menu_list(g, 320, y + 62, 160);
        break;
    }
    case UI_MUSIC: {
        float h = 250, y = 115; card(x, y, w, h);
        gfx_text_center(320, y + 10, 2, rgb(GOLD), "MUSIC");
        const char *now = g->music ? music_now(g->music) : "Music folder unavailable.";
        snprintf(buf, sizeof buf, "%.48s", now);
        gfx_text_center(320, y + 48, 1, rgb(CREAM), buf);
        if (g->music) {
            const char *folder = g->music->folder;
            snprintf(buf, sizeof buf, "%.48s", strlen(folder) > 48 ? folder + strlen(folder) - 48 : folder);
            gfx_text_center(320, y + 66, 1, rgba(CREAM, 0xb0), buf);
        }
        menu_list(g, 320, y + 94, 300);
        break;
    }
    case UI_NONE: break;
    }
}

void hud_draw(const Game *g, const Input *in) {
    gfx_begin_2d();
    bool play_hud = g->phase != PH_MENU && g->phase != PH_INTRO && g->phase != PH_CHAMPION;
    if (play_hud) {
        scoreboard(g);
        if (hud_board_view_visible(g)) board_view_frame();
        wind_panel(g);
        turn_banner(g);
        callout(g);
        shot_arrow(g);
        help_legend(g, in);
        if (g->hold_still_ms > 0) gfx_text_center_shadow(320, 300, 2, rgb(GOLD), "HOLD STILL");
    }
    if (g->ui != UI_NONE) {
        if (g->ui != UI_SETPOWER) gfx_rect(0, 0, 640, 480, rgba(0x000000, 0x50));
        panel(g);
    } else if (g->phase == PH_MENU) panel(g);
    if (g->connect_prompt) {
        float w = 360, h = g->connect_offer_reassign ? 96 : 72, x = 320 - w / 2, y = 200;
        card(x, y, w, h);
        char buf[32]; snprintf(buf, sizeof buf, "CONNECT REMOTE %u", g->connect_remote);
        gfx_text_center(320, y + 14, 2, rgb(GOLD), buf);
        if (g->connect_offer_reassign) gfx_text_center(320, y + 58, 1, rgb(CREAM), "A: USE REMOTE 1");
    }
}

void hud_draw_pointer(float x, float y) {
    /* Gold plus; drawn after the HUD so it stays visible on menus. */
    gfx_rect(x - 10, y - 2, 20, 4, rgb(GOLD));
    gfx_rect(x - 2, y - 10, 4, 20, rgb(GOLD));
    gfx_rect(x - 3, y - 3, 6, 6, rgb(CREAM));
}
