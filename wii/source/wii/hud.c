/* HUD + menus on the 480-high 2D screen: 640 wide at 4:3, 853.3 at 16:9 (UI_W). Left-anchored
 * pieces keep their x, right-anchored ones use UI_W, centred ones UI_CX (SPEC §2.6).
 * Palette: teal #173f3d, cream #fff1cf, gold #ffe16b,
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
#define UI_W gfx_ui_w
#define UI_CX (gfx_ui_w * 0.5f)

static const char *const throw_type_names[] = {"SLIDER", "NORMAL", "KOBE"};

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
    const float w = 360, x = UI_CX - w / 2, y = 14, h = 62;
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
        gfx_text_center(UI_CX, y + 6, 1, rgb(GOLD), "PRACTICE");
        int n = g->practice_throws;
        snprintf(mid, sizeof mid, "%d IN", g->practice_in); gfx_text_center(UI_CX, y + 22, 1, rgb(CREAM), mid);
        snprintf(mid, sizeof mid, "%d ON", g->practice_on); gfx_text_center(UI_CX, y + 38, 1, rgb(CREAM), mid);
        snprintf(mid, sizeof mid, "%d%% HIT " DOT " %d THROWS", n ? (int)lround((g->practice_in + g->practice_on) * 100.0 / n) : 0, n);
        card(UI_CX - gfx_text_width(mid, 1) / 2 - 8, y + h + 4, gfx_text_width(mid, 1) + 16, 22);
        gfx_text_center(UI_CX, y + h + 7, 1, rgb(CREAM), mid);
    } else {
        snprintf(mid, sizeof mid, "ROUND %d", g->inning);
        gfx_text_center(UI_CX, y + 6, 1, rgb(CREAM), mid);
        gfx_text_center(UI_CX, y + 22, 1, rgb(GOLD), g->phase == PH_OVER ? "FINAL" : "TO 21");
        if (g->phase != PH_MENU && g->phase != PH_INTRO && g->phase != PH_CHAMPION) {
            int d = g->live.pts[0] - g->live.pts[1];
            if (!d) snprintf(mid, sizeof mid, "EVEN"); else snprintf(mid, sizeof mid, "%s +%d", d > 0 ? "RED" : "BLUE", d > 0 ? d : -d);
            gfx_text_center(UI_CX, y + 40, 1, d > 0 ? rgb(RED) : d < 0 ? rgb(BLUE) : rgb(CREAM), mid);
        }
    }
}

static void wind_panel(const Game *g) {
    if (g->wind.mph <= 0) return;
    const float w = 84, h = 76, x = UI_W - 28 - w, y = 168;
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
    float cx = hud_board_view_visible(g) ? (UI_CX + 76) : UI_CX;   /* arcade.css #turn {left:200px; top:112px} */
    gfx_text_center_shadow(cx, 106, 2, rgb(t ? BLUE : RED), buf);
    if (g->pass_and_play && g->phase == PH_AIM) gfx_text_center_shadow(cx, 140, 1, rgb(CREAM), "PASS THE REMOTE");
}

static void callout(const Game *g) {
    if (!g->msg_on) return;
    /* arcade.css #msg {left:200px; right:16px}: centred right of the board-view inset */
    float s = strlen(g->msg_big) > 9 ? 2 : 3, cx = hud_board_view_visible(g) ? (UI_CX + 76) : UI_CX;
    gfx_text_center_shadow(cx, 166, s, rgb(0xffffff), g->msg_big);
    if (g->msg_small[0]) gfx_text_center_shadow(cx, 166 + 16 * s + 8, 1, rgb(GOLD), g->msg_small);
}

static void power_gauge(const Game *g) {
    if (!g->last_shot || game_menu_screen(g) || g->connect_prompt) return;
    /* The upper-right space clears the turn banner even at 4:3. Wind and diagnostics sit below. */
    const float w = gfx_aspect > 1.5 ? 168 : 124, h = 46, x = UI_W - 20 - w, y = 94;
    const int percent = game_last_shot_percent(g);
    const double fill = percent / 100.0;
    char label[8];
    snprintf(label, sizeof label, "%d%%", percent);
    card(x, y, w, h);
    gfx_text(x + 8, y + 6, 1, rgb(CREAM), "LAST SHOT");
    gfx_text(x + w - 8 - gfx_text_width(label, 1), y + 6, 1, rgb(GOLD), label);
    float tx = x + 10, ty = y + 26, tw = w - 20, th = 12;
    gfx_rect(tx - 2, ty - 2, tw + 4, th + 4, rgb(0x769184));
    gfx_rect(tx, ty, tw, th, rgb(INK));
    gfx_rect(tx, ty, tw * (float)fill, th, power_color(fill));
    for (int i = 1; i < 5; i++) gfx_rect(tx + tw * i / 5 - 1, ty, 2, th, rgb(TEAL));
}

/* The combined direction + power arrow (arcade 1.0.7 / COMBINED_ARROW_NOTES.md). Geometry comes
 * from core/arrow.c (host-tested): origin at the bag, direction = the launch bearing projected
 * through the main camera, coloured shaft length = power on a faint full-power track. */
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
    int x, y, w, h; view_board_inset(gfx_aspect, &x, &y, &w, &h);
    gfx_rect(x - 3 + 5, y - 22 + 5, w + 6, h + 25, rgba(INK, 0x66));
    gfx_frame(x - 3, y - 22, w + 6, h + 25, 3, rgb(INK));
    gfx_rect(x, y - 19, w, 19, rgb(TEAL));
    gfx_text_center(x + w / 2, y - 18, 1, rgb(GOLD), "BOARD " DOT " 27 FT");
}
void hud_board_view_rect(int *x, int *y, int *w, int *h) { view_board_inset(gfx_aspect, x, y, w, h); }
bool hud_board_view_visible(const Game *g) {
    return g->phase != PH_MENU && g->phase != PH_INTRO && g->phase != PH_CHAMPION &&
           !(g->ui == UI_TITLE || g->ui == UI_SETTINGS || g->ui == UI_SETPOWER || g->ui == UI_CAREER_MAP || g->ui == UI_STRAP);
}

static void help_legend(const Game *g, const Input *in) {
    if (game_menu_screen(g) || g->phase == PH_MENU) return;
    int t = game_thrower(g);
    char status[64];
    const char *state = "-";
    if (in && g->phase == PH_AIM && game_is_human(g, t)) state = in->detector.phase == SWING_READY ? "HOLD B" : "SWING";
    /* 0.1.4 bottom bar: B state, picker (D-pad up/down), surface (A), stance (D-pad left/right) */
    snprintf(status, sizeof status, "%s  \x18\x19 %s  A %s  \x1b\x1a MOVE", state, throw_type_names[g->arc],
             g->side[t] == SIDE_STICKY ? "STICKY" : "SLICK");
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
}
/* 0.1.4: the build is readable on the TV (Retro couldn't read it over ADB). */
static void version_corner(void) {
    const char *v = game_version_tag();
    gfx_text_shadow(UI_W - 10 - gfx_text_width(v, 1), 480 - 22, 1, rgba(CREAM, 0xc0), v);
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
    float w = 420, x = UI_CX - w / 2;
    const Rival *o = &byc_roster[g->fight < 8 ? g->fight : 7];
    Tex *portrait = NULL;
    switch (g->ui) {
    case UI_STRAP: {
        float h = 150, y = 150; card(x, y, w, h);
        gfx_text_center(UI_CX, y + 24, 3, rgb(GOLD), "STRAP ON!");
        gfx_text_center(UI_CX, y + 82, 1, rgb(CREAM), "HOLD B " DOT " BACK " DOT " FORWARD " DOT " LET GO");
        menu_list(g, UI_CX, y + 114, 120);
        gfx_text(x + w - 10 - gfx_text_width(game_version_tag(), 1), y + h - 20, 1, rgba(CREAM, 0xc0), game_version_tag());
        break;
    }
    case UI_TITLE: {
        float h = 300, y = 96; card(x, y, w, h);
        gfx_text_center(UI_CX, y + 14, 1, rgb(GOLD), "WII " DOT " " BYC_VERSION);
        gfx_text_center_shadow(UI_CX, y + 38, 3, rgb(CREAM), "BACKYARD");
        gfx_text_center_shadow(UI_CX, y + 88, 3, rgb(GOLD), "CORNHOLE");
        menu_list(g, UI_CX, y + 158, 220);
        if (!g->save_ok) gfx_text_center(UI_CX, y + h - 22, 1, rgba(CREAM, 0xb0), "Progress won't save on this device.");
        break;
    }
    case UI_PAUSE: {
        float h = 64 + g->menu.count * 24, y = 240 - h / 2; card(x, y, w, h);
        gfx_text_center(UI_CX, y + 12, 2, rgb(GOLD), "PAUSED");
        menu_list(g, UI_CX, y + 52, 260);
        gfx_text(x + w - 8 - gfx_text_width("v" BYC_VERSION, 1), y + h + 6, 1, rgba(CREAM, 0xc0), "v" BYC_VERSION);
        break;
    }
    case UI_SETTINGS: {
        float h = 60 + g->menu.count * 24, y = 240 - h / 2; card(x, y, w, h);
        gfx_text_center(UI_CX, y + 10, 2, rgb(GOLD), "SETTINGS");
        menu_list(g, UI_CX, y + 48, 380);
        break;
    }
    case UI_SETPOWER: {
        float h = 170, y = 150; card(x, y, w, h);
        gfx_text_center(UI_CX, y + 12, 2, rgb(GOLD), g->setpower_slot ? "BLUE SET POWER" : "RED SET POWER");
        gfx_text_center(UI_CX, y + 54, 2, rgb(CREAM), "TOSS COMFORTABLY X3");
        for (int i = 0; i < SWING_CAL_TOSSES; i++) gfx_rect(UI_CX - 40 + i * 30, y + 92, 20, 12, i < g->setpower_count ? rgb(GOLD) : rgba(CREAM, 0x40));
        if (g->setpower_note[0]) gfx_text_center(UI_CX, y + 114, 1, rgb(strcmp(g->setpower_note, "POWER SET!") ? RED : GOLD), g->setpower_note);
        gfx_text_center(UI_CX, y + 140, 1, rgba(CREAM, 0xc0), "HOME: DONE");
        break;
    }
    case UI_CAREER_MAP: {
        float h = 330, y = 74; card(x, y, w, h);
        gfx_text_center(UI_CX, y + 10, 1, rgb(GOLD), "ROAD TO CORNHOLE KING");
        for (int i = 0; i < 8; i++) {
            const char *mark = i < g->fight ? "\xFB" : i == g->fight ? "\x10" : DOT;
            snprintf(buf, sizeof buf, "%s %s " DOT " %s", mark, byc_roster[i].name, byc_circuits[byc_roster[i].circuit]);
            Rgba c = i < g->fight ? rgba(CREAM, 0x90) : i == g->fight ? rgb(GOLD) : rgb(CREAM);
            gfx_text(x + 24, y + 34 + i * 20, 1, c, buf);
        }
        snprintf(buf, sizeof buf, "%d WINS " DOT " %d/8 CLEARED", g->wins, g->fight);
        gfx_text_center(UI_CX, y + 200, 1, rgb(CREAM), buf);
        menu_list(g, UI_CX, y + 228, 220);
        if (!g->save_ok) gfx_text_center(UI_CX, y + h - 22, 1, rgba(CREAM, 0xb0), "Progress won't save on this device.");
        break;
    }
    case UI_INTRO: {
        float h = 330, y = 74; card(x, y, w, h);
        snprintf(buf, sizeof buf, "%s " DOT " MATCH %d/8", byc_circuits[o->circuit], g->fight + 1);
        gfx_text_center(UI_CX, y + 10, 1, rgb(GOLD), buf);
        snprintf(buf, sizeof buf, "NEXT UP: %s", o->name);
        gfx_text_center(UI_CX, y + 32, 1, rgb(CREAM), buf);
        scene_portrait(o->level, &portrait);
        tex_bind(portrait); {
            float px = UI_CX - 40, py = y + 56, s = 80;
            GX_Begin(GX_QUADS, GX_VTXFMT1, 4);
            GX_Position3f32(px, py, -1); GX_Color4u8(255, 255, 255, 255); GX_TexCoord2f32(0, 0);
            GX_Position3f32(px + s, py, -1); GX_Color4u8(255, 255, 255, 255); GX_TexCoord2f32(1, 0);
            GX_Position3f32(px + s, py + s, -1); GX_Color4u8(255, 255, 255, 255); GX_TexCoord2f32(1, 1);
            GX_Position3f32(px, py + s, -1); GX_Color4u8(255, 255, 255, 255); GX_TexCoord2f32(0, 1);
            GX_End();
        }
        snprintf(buf, sizeof buf, "\"%s\"", o->nick); gfx_text_center(UI_CX, y + 146, 1, rgb(CREAM), buf);
        wrap_center(UI_CX, y + 166, w - 30, rgb(CREAM), o->taunt);
        const char *style = o->style; char up[16]; size_t i = 0; for (; style[i] && i < 15; i++) up[i] = (char)(style[i] >= 'a' && style[i] <= 'z' ? style[i] - 32 : style[i]); up[i] = 0;
        snprintf(buf, sizeof buf, "%s " DOT " %s", up, o->circuit == 0 ? "CALM AIR" : o->circuit == 1 ? "LIGHT WIND" : "STRONG WIND");
        gfx_text_center(UI_CX, y + 206, 1, rgb(GOLD), buf);
        menu_list(g, UI_CX, y + 250, 220);
        break;
    }
    case UI_CHAMPION: {
        float h = 300, y = 90; card(x, y, w, h);
        gfx_text_center(UI_CX, y + 12, 1, rgb(GOLD), "ALL FOUR CIRCUITS CONQUERED");
        gfx_text_center_shadow(UI_CX, y + 40, 3, rgb(GOLD), "CORNHOLE KING!");
        scene_portrait(7, &portrait); tex_bind(portrait); {
            float px = UI_CX - 40, py = y + 100, s = 80;
            GX_Begin(GX_QUADS, GX_VTXFMT1, 4);
            GX_Position3f32(px, py, -1); GX_Color4u8(255, 255, 255, 255); GX_TexCoord2f32(0, 0);
            GX_Position3f32(px + s, py, -1); GX_Color4u8(255, 255, 255, 255); GX_TexCoord2f32(1, 0);
            GX_Position3f32(px + s, py + s, -1); GX_Color4u8(255, 255, 255, 255); GX_TexCoord2f32(1, 1);
            GX_Position3f32(px, py + s, -1); GX_Color4u8(255, 255, 255, 255); GX_TexCoord2f32(0, 1);
            GX_End();
        }
        gfx_text_center(UI_CX, y + 190, 1, rgb(CREAM), "All eight rivals defeated.");
        menu_list(g, UI_CX, y + 224, 220);
        break;
    }
    case UI_SUMMARY: case UI_CAREER_RESULT: {
        const RoundResult *r = &g->round;
        float h = 250, y = 110; card(x, y, w, h);
        if (g->ui == UI_CAREER_RESULT) {
            const Rival *rv = &byc_roster[(g->winner == 0 ? g->fight - 1 : g->fight) < 8 ? (g->winner == 0 ? g->fight - 1 : g->fight) : 7];
            gfx_text_center(UI_CX, y + 12, 1, rgb(GOLD), g->winner == 0 ? "CIRCUIT VICTORY" : "RIVAL WINS");
            snprintf(buf, sizeof buf, "%s %s", g->winner == 0 ? "You beat" : "You lost to", rv->name);
            gfx_text_center(UI_CX, y + 44, 1, rgb(CREAM), buf);
            snprintf(buf, sizeof buf, "%d - %d", g->scores[0], g->scores[1]);
            gfx_text_center(UI_CX, y + 68, 2, rgb(CREAM), buf);
            gfx_text_center(UI_CX, y + 110, 1, rgb(CREAM), g->winner == 0 ? "Next rival unlocked!" : "Try again.");
            if (!g->save_ok) gfx_text_center(UI_CX, y + 132, 1, rgba(CREAM, 0xb0), "Progress won't save on this device.");
            menu_list(g, UI_CX, y + 170, 220);
            break;
        }
        if (g->phase == PH_OVER) {
            gfx_text_center(UI_CX, y + 10, 1, rgb(GOLD), "FINAL");
            if (g->vs_cpu) snprintf(buf, sizeof buf, "%s", g->winner == 0 ? "YOU WIN!" : "CPU WINS");
            else snprintf(buf, sizeof buf, "%s WINS!", g->winner ? "BLUE" : "RED");
            gfx_text_center_shadow(UI_CX, y + 30, 3, rgb(g->winner ? BLUE : RED), buf);
        } else {
            snprintf(buf, sizeof buf, "ROUND %d", r->inning); gfx_text_center(UI_CX, y + 10, 1, rgb(GOLD), buf);
            if (r->scorer < 0) snprintf(buf, sizeof buf, "WASH"); else snprintf(buf, sizeof buf, "%s +%d", r->scorer ? "BLUE" : "RED", r->net);
            gfx_text_center_shadow(UI_CX, y + 30, 3, r->scorer < 0 ? rgb(CREAM) : rgb(r->scorer ? BLUE : RED), buf);
        }
        for (int t = 0; t < 2; t++) {
            float cx = t ? UI_CX + 110 : UI_CX - 110;
            gfx_text_center(cx, y + 90, 1, rgb(t ? BLUE : RED), t ? "BLUE" : "RED");
            snprintf(buf, sizeof buf, "%d", r->lie.pts[t]); gfx_text_center(cx, y + 108, 2, rgb(CREAM), buf);
            snprintf(buf, sizeof buf, "%d IN " DOT " %d ON", r->lie.in_c[t], r->lie.on_c[t]); gfx_text_center(cx, y + 142, 1, rgb(CREAM), buf);
        }
        gfx_text_center(UI_CX, y + 112, 1, rgb(CREAM), "vs");
        if (r->scorer < 0) snprintf(buf, sizeof buf, "TIED ROUND " DOT " NO POINTS");
        else snprintf(buf, sizeof buf, "%d - %d = %d (CANCELLATION)", r->lie.pts[r->scorer], r->lie.pts[1 - r->scorer], r->net);
        gfx_text_center(UI_CX, y + 166, 1, rgb(CREAM), buf);
        snprintf(buf, sizeof buf, "MATCH: RED %d " DOT " BLUE %d", g->scores[0], g->scores[1]);
        gfx_text_center(UI_CX, y + 186, 1, rgb(CREAM), buf);
        if (g->phase != PH_OVER) { snprintf(buf, sizeof buf, "%s THROWS FIRST", g->first ? "BLUE" : "RED"); gfx_text_center(UI_CX, y + 204, 1, rgb(GOLD), buf); }
        menu_list(g, UI_CX, y + 224, 200);
        break;
    }
    case UI_CONFIRM_QUIT: case UI_CONFIRM_NEW_CAREER: {
        float h = 130, y = 175; card(x, y, w, h);
        gfx_text_center(UI_CX, y + 16, 2, rgb(GOLD), g->ui == UI_CONFIRM_QUIT ? "QUIT?" : "NEW CAREER?");
        menu_list(g, UI_CX, y + 62, 160);
        break;
    }
    case UI_MUSIC: {
        float h = 250, y = 115; card(x, y, w, h);
        gfx_text_center(UI_CX, y + 10, 2, rgb(GOLD), "MUSIC");
        const char *now = g->music ? music_now(g->music) : "Music folder unavailable.";
        snprintf(buf, sizeof buf, "%.48s", now);
        gfx_text_center(UI_CX, y + 48, 1, rgb(CREAM), buf);
        if (g->music) {
            const char *folder = g->music->folder;
            snprintf(buf, sizeof buf, "%.48s", strlen(folder) > 48 ? folder + strlen(folder) - 48 : folder);
            gfx_text_center(UI_CX, y + 66, 1, rgba(CREAM, 0xb0), buf);
        }
        menu_list(g, UI_CX, y + 94, 300);
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
        power_gauge(g);
        shot_arrow(g);
        help_legend(g, in);
        version_corner();
    }
    if (g->ui != UI_NONE) {
        if (g->ui != UI_SETPOWER) gfx_rect(0, 0, UI_W, 480, rgba(0x000000, 0x50));
        panel(g);
    } else if (g->phase == PH_MENU) panel(g);
    if (g->connect_prompt) {
        float w = 360, h = g->connect_offer_reassign ? 96 : 72, x = UI_CX - w / 2, y = 200;
        card(x, y, w, h);
        char buf[32]; snprintf(buf, sizeof buf, "CONNECT REMOTE %u", g->connect_remote);
        gfx_text_center(UI_CX, y + 14, 2, rgb(GOLD), buf);
        if (g->connect_offer_reassign) gfx_text_center(UI_CX, y + 58, 1, rgb(CREAM), "A: USE REMOTE 1");
    }
}

enum { DEBUG_LINE_HEIGHT = 16, DEBUG_LINES = 12 };

static void debug_line(float x, float *y, unsigned color, const char *text) {
    gfx_text(x, *y, 1, rgb(color), text);
    *y += DEBUG_LINE_HEIGHT;
}

static void draw_wiimote_cal(float x, float y, unsigned remote, const HudWiimoteCalibration *cal) {
    static const struct {
        const char *label, *instruction, *expected;
        SwingVector g;
    } poses[HUD_WIIMOTE_POSES] = {
        {"UP", "HOLD UPRIGHT WITH THE TIP UP", "EXPECT X+0.00 Y-1.00 Z+0.00", {0, -1, 0}},
        {"FLAT", "POINT AT TV WITH BUTTONS UP", "EXPECT X+0.00 Y+0.00 Z+1.00", {0, 0, 1}},
        {"RIGHT", "FROM FLAT, ROLL RIGHT 90 DEG", "DROP RIGHT EDGE  EXPECT X+1.00", {1, 0, 0}},
        {"DOWN", "POINT TIP DOWN, PORT END UP", "EXPECT X+0.00 Y+1.00 Z+0.00", {0, 1, 0}},
        {"LEAN", "LEAN UPRIGHT TIP 30 DEG TOWARD TV", "EXPECT Y-0.87 AND Z+/-0.50", {0, -0.866, 0.50}},
    };
    char line[64];
    snprintf(line, sizeof line, "POSE CALIBRATION  REMOTE %u", remote);
    debug_line(x, &y, GOLD, line);
    debug_line(x, &y, CREAM, "A CAPTURE   LEFT/RIGHT POSE   2 LIVE");
    debug_line(x, &y, CREAM, "POSE     X     Y     Z    |G| AXIS");

    for (unsigned i = 0; i < HUD_WIIMOTE_POSES; ++i) {
        const char focus = i == cal->pose ? '>' : ' ';
        unsigned color = CREAM;
        if (cal->captured[i]) {
            SwingVector g = cal->poses[i], target = poses[i].g;
            double length = sqrt(g.x * g.x + g.y * g.y + g.z * g.z);
            char axis = 'X';
            double dominant = g.x;
            if (fabs(g.y) > fabs(dominant)) { dominant = g.y; axis = 'Y'; }
            if (fabs(g.z) > fabs(dominant)) { dominant = g.z; axis = 'Z'; }
            double z = i == HUD_WIIMOTE_POSES - 1 ? fabs(g.z) : g.z;
            bool pass = fabs(g.x - target.x) <= .15 && fabs(g.y - target.y) <= .15 &&
                        fabs(z - target.z) <= .15 && fabs(length - 1) <= .15;
            snprintf(line, sizeof line, "%c%-5s %+.2f %+.2f %+.2f %.2f %c%c %s",
                     focus, poses[i].label, g.x, g.y, g.z, length, dominant < 0 ? '-' : '+',
                     axis, pass ? "PASS" : "FAIL");
            color = pass ? 0xa7efb1 : RED;
        } else snprintf(line, sizeof line, "%c%-5s  -", focus, poses[i].label);
        debug_line(x, &y, color, line);
    }

    debug_line(x, &y, GOLD, poses[cal->pose].instruction);
    debug_line(x, &y, CREAM, poses[cal->pose].expected);
    if (cal->status == HUD_POSE_CAPTURING) debug_line(x, &y, GOLD, "HOLD STILL  CAPTURING 300 MS");
    else if (cal->status == HUD_POSE_INTERRUPTED) debug_line(x, &y, RED, "REPORT GAP  A RETRY");
    else if (cal->status == HUD_POSE_MOVED) debug_line(x, &y, RED, "HOLD STILL NEAR 1 G  A RETRY");
    else if (cal->pose == HUD_WIIMOTE_POSES - 1)
        debug_line(x, &y, CREAM, "Z- BUTTONS FACE TV  Z+ FACE YOU");
}

static int swing_percent(double power) {
    return (int)lround(100 * power / SWING_POWER_CAP);
}

void hud_draw_wiimote_debug(const Input *in, const InputSample *latest,
                           const HudWiimoteCalibration *cal, bool static_pose) {
    const Swing *s = &in->detector;
    const InputStats *st = &in->stats;
    const InputSlot *slot = &in->slots[in->active_slot];
    unsigned remote = in->channels[in->active_slot] + 1;
    /* At 4:3 the box starts at x=324, just clear of the expanded help at x=322.
     * Its top clears the wind card, and its bottom clears the help legend. */
    const float w = 304, h = DEBUG_LINES * DEBUG_LINE_HEIGHT + 8;
    float x = UI_W - 12 - w, y = 434 - h;
    gfx_rect(x, y, w, h, rgba(TEAL, 0xf0));
    x += 4;
    y += 4;
    if (cal->active) {
        draw_wiimote_cal(x, y, remote, cal);
        return;
    }

    char line[72];
    const SwingSample *p = &latest->motion;
    SwingVector g = p->g;
    if (latest->motion.accel_valid) {
        snprintf(line, sizeof line, "ACCEL G  X %+.2f  Y %+.2f  Z %+.2f", g.x, g.y, g.z);
    } else snprintf(line, sizeof line, "ACCEL G  NO REPORT");
    debug_line(x, &y, CREAM, line);

    char speed[12] = "-", peak[12] = "-", preview[8] = "-";
    if (swing_armed(s) && s->observer_valid) snprintf(speed, sizeof speed, "%.2f", s->speed);
    double peak_g = swing_recent_peak_g(s);
    if (swing_armed(s) && peak_g > 0) snprintf(peak, sizeof peak, "%.2f", peak_g);
    snprintf(line, sizeof line, "SPEED %s M/S  PEAK %s G", speed, peak);
    debug_line(x, &y, CREAM, line);

    double power, aim;
    if (swing_preview(s, &slot->profile, slot->height, &power, &aim))
        snprintf(preview, sizeof preview, "%d%%", swing_percent(power));
    double medium_g = swing_medium_g(slot->height, slot->profile.effort_scale);
    snprintf(line, sizeof line, "%s MED %.2f G LIVE %s", throw_type_names[slot->height], medium_g, preview);
    debug_line(x, &y, GOLD, line);

    if (st->have_toss)
        snprintf(line, sizeof line, "LAST PEAK %.2f G  %.2f M/S", st->last_toss.peak_g, st->last_toss.speed);
    else snprintf(line, sizeof line, "LAST THROW  -");
    debug_line(x, &y, GOLD, line);
    if (st->have_toss) {
        snprintf(line, sizeof line, "LAST P%d%% A%+.2f S%+.2f", swing_percent(st->last_toss.power), st->last_toss.aim, st->last_toss.spin);
        debug_line(x, &y, GOLD, line);
    }

    if (p->accel_valid) {
        bool live = s->have_time && s->last_ms == p->ms;
        double pitch = live ? s->pitch : atan2(-g.y, hypot(g.x, g.z)) * 180 / SWING_PI;
        double face = live ? s->face_pitch : atan2(-g.y, g.z) * 180 / SWING_PI;
        double roll = live ? s->roll : atan2(g.x, g.z) * 180 / SWING_PI;
        snprintf(line, sizeof line, "PITCH%+.0f %s FACE%+.0f ROLL%+.0f",
                 pitch, (live ? s->pitch_static : static_pose) ? "STATIC" : "MOVING", face, roll);
    } else snprintf(line, sizeof line, "PITCH  -");
    debug_line(x, &y, CREAM, line);

    const char *b = !latest->buttons_valid ? "?" : p->b ? "HELD" : "UP";
    snprintf(line, sizeof line, "PHASE %s B %s P%u A POSES", swing_phase_name(s), b,
             (unsigned)fmin(99, s->passes));
    debug_line(x, &y, GOLD, line);
    snprintf(line, sizeof line, "REJECT %s", swing_reject_name(s->last_reject));
    debug_line(x, &y, s->last_reject == SWING_REJECT_NONE ? CREAM : GOLD, line);

    if (!p->accel_valid) snprintf(line, sizeof line, "TAN - RAD - LAT -");
    else if (s->have_axis) {
        if (s->sign_source != SWING_SIGN_UNKNOWN)
            snprintf(line, sizeof line, "TAN %+.2f RAD %+.2f LAT %+.2f E%+.2f",
                     s->tangent, s->radial, g.x, s->excess);
        else snprintf(line, sizeof line, "TAN - RAD %+.2f LAT %+.2f E %+.2f", s->radial, g.x, s->excess);
    } else snprintf(line, sizeof line, "TAN - RAD - LAT %+.2f", g.x);
    debug_line(x, &y, CREAM, line);

    char arm[12] = "-", bottom[12] = "-";
    if (s->observer_valid) snprintf(arm, sizeof arm, "%+.0f", s->arm_angle);
    if (s->have_bottom) {
        uint64_t ago = p->ms >= s->bottom_ms ? p->ms - s->bottom_ms : 0;
        snprintf(bottom, sizeof bottom, "+%uMS", (unsigned)(ago < 99999 ? ago : 99999));
    }
    snprintf(line, sizeof line, "FIT %.2f G ARM%s BOT%s%s", s->fitted_g, arm, bottom,
             p->accel_valid && p->clipped ? " CLIP" : "");
    debug_line(x, &y, CREAM, line);

    const unsigned *r = s->rejects;
    snprintf(line, sizeof line, "B%u E%u W%u L%u N%u G%u %u/S %uMS",
             (unsigned)fmin(99, r[SWING_REJECT_BACKSWING]), (unsigned)fmin(99, r[SWING_REJECT_BEFORE_BOTTOM]),
             (unsigned)fmin(99, r[SWING_REJECT_TOO_WEAK]), (unsigned)fmin(99, r[SWING_REJECT_TOO_LATE]),
             (unsigned)fmin(99, r[SWING_REJECT_NO_SWING]), (unsigned)fmin(99, r[SWING_REJECT_REPORT_GAP]),
             (unsigned)fmin(999, st->rate), (unsigned)fmin(9999, input_largest_gap(in)));
    debug_line(x, &y, CREAM, line);

    if (s->have_axis) {
        const char *source = s->sign_source == SWING_SIGN_GRIP ? "GRIP" :
                             s->sign_source == SWING_SIGN_TURN ? "TURN" : "-";
        char sign = s->sign_source == SWING_SIGN_UNKNOWN ? '?' : s->tangent_sign > 0 ? '+' : '-';
        snprintf(line, sizeof line, "AXES Y%+.2f Z%+.2f T%c%s G%+.0f D%.0f",
                 s->ny, s->nz, sign, source, s->grip_pitch, s->grip_turn);
    } else snprintf(line, sizeof line, "AXES  -");
    debug_line(x, &y, CREAM, line);
}
