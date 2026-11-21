#include "scene.h"
#include "../core/wind_streaks.h"
#include "party_draw.h"
#include <ogc/lwp_watchdog.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ---------------- textures (ports of the 1.0.3 pixTex painters) ---------------- */
static Tex tex_grass, tex_board, tex_hay, tex_bag[2][2], tex_portrait[BYC_RIVALS];
static unsigned team1_color;
static DList yard;

/* index.html R = rng(7) is shared by pick() in the hay texture and by boardTexture: replay the same
 * draw order (hay first: 128 picks) so the board speckle matches 1.0.3 pixel for pixel. */
static uint32_t r7_state = 7;
static double R7(void) {
    uint32_t s = r7_state += 0x6D2B79F5u;
    uint32_t t = (s ^ (s >> 15)) * (1u | s);
    t = (t + ((t ^ (t >> 7)) * (61u | t))) ^ t;
    return (double)(t ^ (t >> 14)) / 4294967296.0;
}
static void paint_hay(void) {
    static const unsigned c[4] = {0xf2cf6b, 0xe8c05a, 0xf7da7e, 0xd9ad48};
    tex_create(&tex_hay, 16, 8, true);
    for (int y = 0; y < 8; y++) for (int x = 0; x < 16; x++) tex_set(&tex_hay, x, y, rgb(c[(int)floor(R7() * 4)]));
    for (int x = 0; x < 16; x++) { tex_set(&tex_hay, x, 2, rgb(0xb98a34)); tex_set(&tex_hay, x, 5, rgb(0xb98a34)); }
    tex_upload(&tex_hay);
}
static void paint_board(void) {
    const unsigned main_c = 0xe8c28c, accent = 0x1aa39a, shade_c = 0xddb47c, sun_top = 0xffe45e, sun_bot = 0xff8a3d;
    Tex *t = &tex_board;
    tex_create(t, 32, 64, true);
    for (int y = 0; y < 64; y++) for (int x = 0; x < 32; x++) tex_set(t, x, y, rgb(R7() < 0.12 ? shade_c : main_c));
    tex_fill(t, 0, 0, 32, 2, rgb(0xfff4dc)); tex_fill(t, 0, 62, 32, 2, rgb(0xfff4dc));
    tex_fill(t, 0, 0, 2, 64, rgb(0xfff4dc)); tex_fill(t, 30, 0, 2, 64, rgb(0xfff4dc));
    const double hx = 15.5, hy = 64 * (BYC_HOLE_FROM_TOP / BYC_L);
    for (int y = 0; y < 30; y++) for (int x = 0; x < 32; x++) {
        double d = hypot(x + 0.5 - hx - 0.5, y + 0.5 - hy);
        if (d < 11.5 && !(y > hy + 2 && ((int)floor(y - hy) % 3 == 0))) {
            double k = fmax(0, fmin(1, (y - (hy - 11)) / 22));
            tex_set(t, x, y, rgb(k < 0.5 ? sun_top : sun_bot));
        }
    }
    for (int y = 0; y < 64; y++) for (int x = 0; x < 32; x++) {
        double d = hypot(x + 0.5 - hx - 0.5, y + 0.5 - hy);
        if (d > 4.3 && d < 5.6) tex_set(t, x, y, rgb(0xfff4dc));
    }
    for (int i = 0; i < 3; i++) tex_fill(t, 2, 40 + i * 3, 28, 2, rgb(i == 1 ? 0xfff4dc : accent));
    static const char *star[5] = {"..#..", "..#..", "#####", ".###.", ".#.#."};
    for (int y = 0; y < 5; y++) for (int x = 0; x < 5; x++) if (star[y][x] == '#') tex_set(t, 13 + x, 52 + y, rgb(0xffe45e));
    tex_upload(t);
}
static void paint_bag(Tex *t, unsigned c, unsigned dark, unsigned light, bool sticky, bool fresh) {
    if (fresh) tex_create(t, 8, 8, true);
    for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++)
        tex_set(t, x, y, rgb(sticky ? ((x * 3 + y * 5) % 7 == 0 ? dark : c) : ((x + y) % 3 == 0 ? light : c)));
    for (int i = 0; i < 8; i++) { tex_set(t, i, 0, rgb(dark)); tex_set(t, i, 7, rgb(dark)); tex_set(t, 0, i, rgb(dark)); tex_set(t, 7, i, rgb(dark)); }
    for (int i = 1; i < 7; i += 2) { tex_set(t, i, 1, rgb(0xfff4dc)); tex_set(t, i, 6, rgb(0xfff4dc)); }
    tex_upload(t);
}
static void paint_team1(unsigned color, bool fresh) {
    if (color == 0x2f5fe8) { paint_bag(&tex_bag[1][0], 0x2f5fe8, 0x1a2a80, 0x6b93ff, false, fresh); paint_bag(&tex_bag[1][1], 0x2f5fe8, 0x1a2a80, 0x6b93ff, true, fresh); }
    else { paint_bag(&tex_bag[1][0], color, 0x252237, 0xffffff, false, fresh); paint_bag(&tex_bag[1][1], color, 0x252237, 0xffffff, true, fresh); }
    team1_color = color;
}
static void paint_grass(void) {
    /* backyard.js grassMap: base + 125 random strokes (its own random; seeded here). */
    static const unsigned c[4] = {0x689c4c, 0x6c9e4e, 0x619447, 0x709f51};
    Rng r; rng_seed(&r, 99);
    tex_create(&tex_grass, 64, 64, true);
    tex_fill(&tex_grass, 0, 0, 64, 64, rgb(0x65984a));
    for (int i = 0; i < 125; i++) {
        Rgba col = rgb(c[(int)(rng_next(&r) * 4)]);
        int x = (int)(rng_next(&r) * 64), y = (int)(rng_next(&r) * 64), w = 2 + (int)(rng_next(&r) * 6), h = 1 + (int)(rng_next(&r) * 3);
        for (int j = 0; j < h; j++) for (int k = 0; k < w; k++) tex_set(&tex_grass, (x + k) % 64, (y + j) % 64, col);
    }
    tex_upload(&tex_grass);
}

/* portrait(): 16x16 SVG made of rectilinear M/h/v/H/V/z paths; even-odd fill at pixel centres. */
static void fill_path(Tex *t, const char *d, Rgba c) {
    float px[64], py[64]; int n = 0, start = 0;
    float x = 0, y = 0;
    float polys[8][2]; (void)polys;
    int poly_start[16], poly_count = 0;
    const char *s = d;
    while (*s) {
        char cmd = *s++;
        float a = 0, b = 0; char *end;
        switch (cmd) {
        case 'M': a = strtof(s, &end); s = end; b = strtof(s, &end); s = end; x = a; y = b; start = n; poly_start[poly_count++] = n; px[n] = x; py[n] = y; n++; break;
        case 'h': x += strtof(s, &end); s = end; px[n] = x; py[n] = y; n++; break;
        case 'v': y += strtof(s, &end); s = end; px[n] = x; py[n] = y; n++; break;
        case 'H': x = strtof(s, &end); s = end; px[n] = x; py[n] = y; n++; break;
        case 'V': y = strtof(s, &end); s = end; px[n] = x; py[n] = y; n++; break;
        case 'z': x = px[start]; y = py[start]; break;
        default: break;
        }
        if (n >= 63) break;
    }
    poly_start[poly_count] = n;
    for (int yy = 0; yy < 16; yy++) for (int xx = 0; xx < 16; xx++) {
        float cx = xx + 0.5f, cy = yy + 0.5f; bool inside = false;
        for (int p = 0; p < poly_count; p++) {
            int a0 = poly_start[p], a1 = poly_start[p + 1];
            for (int i = a0, j = a1 - 1; i < a1; j = i++)
                if (((py[i] > cy) != (py[j] > cy)) && (cx < (px[j] - px[i]) * (cy - py[i]) / (py[j] - py[i]) + px[i])) inside = !inside;
        }
        if (inside) tex_set(t, xx, yy, c);
    }
}
static void paint_portraits(void) {
    static const char *looks[8][4] = {
        {"f5be91", "633b29", "M4 3h8v3H4z", "M3 3h10v2H3zM5 2h6v1H5z"},
        {"b97851", "241926", "M3 4h10v9h-3V5H6v8H3z", "M3 4h10v1H3zM11 5h3v1h-3z"},
        {"df9c70", "c07431", "M5 2h2v3H5zM8 1h2v4H8zM11 3h1v3h-1z", "M4 5h8v1H4z"},
        {"805039", "151521", "M3 3h9v2H3zM3 5h2v8H3zM11 4h2v10h-2z", "M5 6h3v2H5zM9 6h3v2H9zM8 6h1v1H8z"},
        {"edb18c", "c9c5b5", "M4 3h8v2H4zM3 5h2v3H3zM11 5h2v3h-2z", "M5 9h6v3H5zM6 12h4v1H6z"},
        {"c78e65", "8d294c", "M3 2h4v3H3zM4 3h8v3H4zM12 5h2v5h-2z", "M4 4h8v1H4zM11 5h2v2h-2z"},
        {"694536", "ded39a", "M4 2h8v3H4zM4 5h1v4H4zM11 5h1v4h-1z", "M3 2h10v3H3zM2 5h12v1H2z"},
        {"f1caa5", "eee5f5", "M3 4h10v10h-3V5H6v9H3z", "M4 1h2v2h1V0h2v3h1V1h2v4H4z"},
    };
    for (int i = 0; i < 8; i++) {
        Tex *t = &tex_portrait[i];
        tex_create(t, 16, 16, false);
        unsigned skin = (unsigned)strtoul(looks[i][0], NULL, 16), hair = (unsigned)strtoul(looks[i][1], NULL, 16);
        tex_fill(t, 0, 0, 16, 16, rgb(0x252237));
        fill_path(t, "M2 12h12v4H2z", rgb(byc_roster[i].color));
        fill_path(t, looks[i][2], rgb(hair));
        fill_path(t, "M5 5h6v6H5zM6 11h4v2H6z", rgb(skin));
        fill_path(t, "M6 6h1v1H6zM9 6h1v1H9zM7 9h3v1H7z", rgb(0x272034));
        unsigned acc = i == 4 ? hair : i == 3 ? 0x191c31 : byc_roster[i].color;
        fill_path(t, looks[i][3], rgb(acc));
        tex_upload(t);
    }
}
void scene_portrait(int level, Tex **out) { *out = &tex_portrait[level < 0 ? 0 : level > 7 ? 7 : level]; }

/* ---------------- static yard (backyard.js voxels, solid average colours) ---------------- */
static void voxel(float w, float h, float d, unsigned color, float x, float y, float z) {
    gfx_box((V3){x, y, z}, (V3){w, h, d}, rgb(color), NULL);
}
static M3 board_rot3(void) {
    M3 m = {{{1, 0, 0}, {0, (float)cos(BYC_TH), (float)-sin(BYC_TH)}, {0, (float)sin(BYC_TH), (float)cos(BYC_TH)}}};
    return m;
}
static V3 deck_world(float x, float y, float z) {
    Vec3 w = board_to_world((Vec3){x, y, z});
    return (V3){(float)w.x, (float)w.y, (float)w.z};
}
static void build_yard(void) {
    const float farZ = -13, nearZ = 4, propZ = -7;
    const unsigned fenceWood = 0xb48653, woodDark = 0x6e4a32, woodEnd = 0xd7aa70, bark = 0x6c4a35, birch = 0xc9c4a4;
    const unsigned leaves[3] = {0x41794a, 0x5a9145, 0x77a04f};
    dl_begin(&yard, 1024 * 1024);
    for (int side = 0; side < 2; side++) {   /* fence(farZ); fence(nearZ) */
        float z = side ? nearZ : farZ;
        for (float x = -6.8f; x <= 6.8f; x += .255f) voxel(.22f, 1.22f, .075f, fenceWood, x, .61f, z);
        voxel(13.9f, .085f, .11f, woodDark, 0, .34f, z + .065f); voxel(13.9f, .085f, .11f, woodDark, 0, .94f, z + .065f);
        for (float x = -6.75f; x <= 6.8f; x += 2.25f) { voxel(.16f, 1.37f, .16f, woodDark, x, .685f, z); voxel(.22f, .08f, .22f, woodEnd, x, 1.41f, z); }
    }
    for (int s = -1; s <= 1; s += 2) {
        float x = 6.8f * s;
        for (float z = farZ; z <= nearZ; z += .255f) voxel(.075f, 1.22f, .22f, fenceWood, x, .61f, z);
        voxel(.11f, .085f, nearZ - farZ, woodDark, x - s * .065f, .34f, (farZ + nearZ) / 2);
        voxel(.11f, .085f, nearZ - farZ, woodDark, x - s * .065f, .94f, (farZ + nearZ) / 2);
        for (float z = farZ; z <= nearZ; z += 2.25f) { voxel(.16f, 1.37f, .16f, woodDark, x, .685f, z); voxel(.22f, .08f, .22f, woodEnd, x, 1.41f, z); }
    }
    static const float trees[8][3] = {{-4.8f, -13.8f, 1.05f}, {-1.8f, -14.6f, .7f}, {3.5f, -14.2f, 1.15f}, {5.5f, -13.5f, .9f},
                                      {-4.5f, 4.9f, 1.1f}, {3.8f, 5.0f, 1.15f}, {-4.8f, -7.4f, .9f}, {4.8f, -8.4f, 1}};
    static const float canopy[4][6] = {{0, 0, 0, 1.5f, .95f, 1.4f}, {-.58f, .19f, .18f, .92f, .73f, .95f}, {.56f, .27f, -.21f, .96f, .82f, 1.02f}, {.05f, .76f, .08f, 1.08f, .72f, 1.04f}};
    for (int i = 0; i < 8; i++) {
        float x = trees[i][0], z = trees[i][1], s = trees[i][2];
        unsigned trunk = i % 3 == 1 ? birch : bark;
        voxel(.28f * s, 2.35f * s, .28f * s, trunk, x, 1.175f * s, z);
        voxel(.15f * s, .58f * s, .16f * s, trunk, x + .2f * s, 1.65f * s, z + .08f * s);
        for (int k = 0; k < 4; k++)
            voxel(canopy[k][3] * s, canopy[k][4] * s, canopy[k][5] * s, leaves[i % 3], x + canopy[k][0] * s, 2.25f * s + canopy[k][1] * s, z + canopy[k][2] * s);
    }
    for (int s = -1; s <= 1; s += 2) {
        for (int i = 0; i < 5; i++) {
            float x = s * (1.8f + i * .72f), z = farZ + .55f + (i % 2) * .22f;
            voxel(.66f, .33f + (i % 2) * .12f, .53f, leaves[i % 3], x, .17f + (i % 2) * .06f, z);
        }
        for (int i = 0; i < 7; i++) voxel(.37f, .045f, .38f, i % 2 ? 0x89917d : 0x737d6e, s * 2.05f, .0225f, propZ + 1.1f + i * .47f);
    }
    /* charcoal grill + shelf */
    const float gx = 2.75f, gz = propZ - 1.35f;
    for (int a = -1; a <= 1; a += 2) for (int b = -1; b <= 1; b += 2) voxel(.035f, .45f, .035f, 0x414f49, gx + a * .2f, .225f, gz + b * .16f);
    voxel(.59f, .2f, .46f, 0x303a37, gx, .54f, gz); voxel(.57f, .06f, .44f, 0x485550, gx, .675f, gz);
    for (int i = 0; i < 6; i++) voxel(.025f, .018f, .36f, 0x414f49, gx - .2f + i * .08f, .713f, gz);
    voxel(.3f, .045f, .39f, 0xbe8b54, gx + .49f, .57f, gz); voxel(.14f, .025f, .17f, 0xeee0ba, gx + .49f, .61f, gz + .05f);
    /* board: skirts, legs, dark floor under the hole (index.html makeBoard) */
    M3 r = board_rot3();
    const float W = (float)BYC_W, L = (float)BYC_L, sk = .09f;
    const unsigned skirt = 0xd9a066;
    gfx_box(deck_world(-W / 2 + .0125f, -sk / 2 - .02f, -L / 2), (V3){.025f, sk, L}, rgb(skirt), &r);
    gfx_box(deck_world(W / 2 - .0125f, -sk / 2 - .02f, -L / 2), (V3){.025f, sk, L}, rgb(skirt), &r);
    gfx_box(deck_world(0, -sk / 2 - .02f, -.0125f), (V3){W, sk, .025f}, rgb(skirt), &r);
    gfx_box(deck_world(0, -sk / 2 - .02f, -L + .0125f), (V3){W, sk, .025f}, rgb(skirt), &r);
    gfx_box(deck_world(0, -.1f, -L / 2), (V3){W - .05f, .002f, L - .05f}, rgb(0x1c120c), &r);
    const float legH = (float)BYC_BACK_H + .02f;
    for (int s = -1; s <= 1; s += 2)
        voxel(.05f, legH, .05f, skirt, s * (W / 2 - .05f), legH / 2 - .05f, (float)(BYC_BOARD_Z - BYC_L * cos(BYC_TH) + .08));
    party_draw_static();   /* keg body, beer pong table, perimeter cacti (party.mjs) */
    dl_end(&yard);
}

/* ---------------- dynamic pieces ---------------- */
typedef struct { V3 p, v; float life; Rgba c; bool plain; } Particle;
static Particle confetti[70], dust[14];
typedef struct { V3 p; M3 r; float life; int team; } Ghost;
static Ghost trail[3];
static int trail_index;
static float trail_time;
static float smoke_t[5];
static unsigned vis_seed = 12345;
static float vrand(void) { vis_seed = vis_seed * 1103515245u + 12345u; return (float)((vis_seed >> 8) & 0xFFFF) / 65536.0f; }

typedef struct { int index, count; float elapsed; bool active; } DogParty;
static DogParty dog_party = {-1, 0, 0, false};
static float dog_head_y_rot;
static double scene_time;
static WindStreaks streaks;
static Party party;
static V3 dog_mouth = {2.3f, .5f, -8.9f};

void scene_init(void) {
    physics_init();
    paint_hay();     /* consumes R7 draws first, exactly like index.html */
    paint_board();
    paint_grass();
    paint_bag(&tex_bag[0][0], 0xe8423c, 0x8c1f2a, 0xff6b5e, false, true);
    paint_bag(&tex_bag[0][1], 0xe8423c, 0x8c1f2a, 0xff6b5e, true, true);
    paint_team1(0x2f5fe8, true);
    paint_portraits();
    build_yard();
    wind_streaks_init(&streaks);
    party_init(&party, (uint32_t)gettime());   /* Math.random in the arcade: any seed */
    for (int i = 0; i < 5; i++) smoke_t[i] = i / 5.0f;
}

void scene_fx(const Fx *f) {
    V3 at = {(float)f->at.x, (float)f->at.y, (float)f->at.z};
    static const unsigned cc[6] = {0xffe45e, 0xff6a4d, 0x7ee081, 0xff8fc0, 0x9ff0e6, 0xffffff};
    if (f->kind == FX_HOLE_BURST) {
        for (int i = 0; i < 70; i++) {
            Particle *c = &confetti[i];
            c->p = at; c->v = (V3){(vrand() - .5f) * 2.4f, 1.5f + vrand() * 2.5f, (vrand() - .5f) * 2.4f};
            c->life = 1 + vrand() * .6f; c->c = rgb(cc[i % 6]);
        }
    } else if (f->kind == FX_DUST) {
        for (int i = 0; i < 14; i++) {
            Particle *c = &dust[i];
            c->p = at; c->v = (V3){(vrand() - .5f) * .65f, .3f + vrand() * .5f, (vrand() - .5f) * .65f};
            c->life = .4f;
        }
    } else if (f->kind == FX_CELEBRATE) {
        dog_party.index = (dog_party.index + 1) % 3; dog_party.count++; dog_party.elapsed = 0; dog_party.active = true;
    } else if (f->kind == FX_DOG_RESET) {
        dog_party.active = false; dog_party.elapsed = 0;
    }
}

static M3 bag_rotation(const Bag *b, bool *ok);
void scene_update(const Game *g, double dt) {
    if (g->paused) return;
    scene_time += dt;
    if (g->rival_color != team1_color && g->rival_color) paint_team1(g->rival_color, false);
    for (int i = 0; i < 70; i++) {
        Particle *c = &confetti[i];
        if (c->life <= 0) continue;
        c->life -= dt; c->v.y -= 6 * dt;
        c->p.x += c->v.x * dt; c->p.y += c->v.y * dt; c->p.z += c->v.z * dt;
        if (c->p.y < .02f) { c->p.y = .02f; c->v = (V3){0, 0, 0}; }
    }
    for (int i = 0; i < 14; i++) {
        Particle *c = &dust[i];
        if (c->life <= 0) continue;
        c->life = fmaxf(0, c->life - dt); c->v.y -= 2 * dt;
        c->p.x += c->v.x * dt; c->p.y += c->v.y * dt; c->p.z += c->v.z * dt;
    }
    for (int i = 0; i < 5; i++) smoke_t[i] = fmodf(smoke_t[i] + dt * .25f, 1);
    party_update(&party, dt);
    wind_streaks_update(&streaks, dt, &g->wind, g->phase != PH_MENU && g->phase != PH_INTRO && g->phase != PH_CHAMPION);
    if (dog_party.active) { dog_party.elapsed = fminf(2.2f, dog_party.elapsed + dt); if (dog_party.elapsed >= 2.2f) dog_party.active = false; }
    /* flight trail: three afterimages sampled every 60 ms */
    for (int i = 0; i < 3; i++) trail[i].life = fmaxf(0, trail[i].life - dt);
    const Bag *b = g->active >= 0 ? &g->bags[g->active] : NULL;
    if (!b || b->state != BAG_FLY) trail_time = 0;
    else {
        trail_time += dt;
        if (trail_time >= .06f) {
            trail_time = fmodf(trail_time, .06f);
            Ghost *gh = &trail[trail_index]; trail_index = (trail_index + 1) % 3;
            bool ok; gh->r = bag_rotation(b, &ok); gh->p = (V3){(float)b->p.x, (float)b->p.y, (float)b->p.z}; gh->team = b->team; gh->life = .18f;
        }
    }
    /* dog head tracks the active bag or the hole */
    Vec3 watch = b ? b->p : byc_board.hole_world;
    float dx = (float)watch.x - 2.3f, dz = (float)watch.z + 8.7f;
    float c = cosf(-2.6f), s = sinf(-2.6f);
    float lx = (c * dx + s * dz) / .85f, lz = (-s * dx + c * dz) / .85f;
    float want = fmaxf(-.7f, fminf(.7f, atan2f(-lx, -lz) * .6f));
    dog_head_y_rot += (want - dog_head_y_rot) * fminf(1, (float)dt * 5);
}

/* ---------------- bags ---------------- */
#define BAG_SEG 4
static void bag_vertex(int face, float u, float v, V3 *out) {
    /* BoxGeometry(BAG, 2*BAG_H, BAG, 4,1,4) puffed: y *= 1 - .45*(x^2+z^2)/2 (normalized x,z) */
    float x = 0, y = 0, z = 0;
    switch (face) {
    case 0: x = 1; z = u * 2 - 1; y = v * 2 - 1; break;   /* +x */
    case 1: x = -1; z = u * 2 - 1; y = v * 2 - 1; break;  /* -x */
    case 2: y = 1; x = u * 2 - 1; z = v * 2 - 1; break;   /* +y top */
    case 3: y = -1; x = u * 2 - 1; z = v * 2 - 1; break;  /* -y bottom */
    case 4: z = 1; x = u * 2 - 1; y = v * 2 - 1; break;   /* +z */
    default: z = -1; x = u * 2 - 1; y = v * 2 - 1; break; /* -z */
    }
    float puff = 1 - .45f * (x * x + z * z) / 2;
    *out = (V3){x * (float)BYC_BAG / 2, y * (float)BYC_BAG_H * puff, z * (float)BYC_BAG / 2};
}
static void draw_bag_faces(const M3 *r, Rgba tint, bool down_only, bool skip_down, bool lit) {
    for (int f = 0; f < 6; f++) {
        if (down_only && f != 3) continue;
        if (skip_down && f == 3) continue;
        int su = (f == 2 || f == 3 || f == 4 || f == 5 || f == 0 || f == 1) ? BAG_SEG : BAG_SEG;
        int sv = (f == 2 || f == 3) ? BAG_SEG : 1;
        GX_Begin(GX_QUADS, GX_VTXFMT1, (u16)(su * sv * 4));
        for (int i = 0; i < su; i++) for (int j = 0; j < sv; j++) {
            float u0 = (float)i / su, u1 = (float)(i + 1) / su, v0 = (float)j / sv, v1 = (float)(j + 1) / sv;
            V3 a, b, c, d;
            bag_vertex(f, u0, v0, &a); bag_vertex(f, u1, v0, &b); bag_vertex(f, u1, v1, &c); bag_vertex(f, u0, v1, &d);
            V3 e1 = {b.x - a.x, b.y - a.y, b.z - a.z}, e2 = {d.x - a.x, d.y - a.y, d.z - a.z};
            V3 n = {e1.y * e2.z - e1.z * e2.y, e1.z * e2.x - e1.x * e2.z, e1.x * e2.y - e1.y * e2.x};
            float len = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z);
            if (len > 0) { n.x /= len; n.y /= len; n.z /= len; }
            V3 out = {a.x * 2 / (float)BYC_BAG, a.y, a.z * 2 / (float)BYC_BAG};
            if (n.x * out.x + n.y * out.y * 40 + n.z * out.z < 0) { n.x = -n.x; n.y = -n.y; n.z = -n.z; }
            V3 wn = m3_apply(r, n);
            Rgba col = lit ? shade(tint, wn) : tint;
            GX_Position3f32(a.x, a.y, a.z); GX_Color4u8(col.r, col.g, col.b, col.a); GX_TexCoord2f32(u0, v0);
            GX_Position3f32(b.x, b.y, b.z); GX_Color4u8(col.r, col.g, col.b, col.a); GX_TexCoord2f32(u1, v0);
            GX_Position3f32(c.x, c.y, c.z); GX_Color4u8(col.r, col.g, col.b, col.a); GX_TexCoord2f32(u1, v1);
            GX_Position3f32(d.x, d.y, d.z); GX_Color4u8(col.r, col.g, col.b, col.a); GX_TexCoord2f32(u0, v1);
        }
        GX_End();
    }
}
/* Material rule (makeBagMesh): the chosen surface is thrown DOWN; top + sides show the other one. */
static void draw_bag_mesh(V3 p, const M3 *r, float scale, int team, BagSide side) {
    gfx_model(p, r, (V3){scale, scale, scale});
    tex_bind(&tex_bag[team][side == SIDE_SLICK ? 1 : 0]);
    draw_bag_faces(r, (Rgba){255, 255, 255, 255}, false, true, true);
    tex_bind(&tex_bag[team][side]);
    draw_bag_faces(r, (Rgba){255, 255, 255, 255}, true, false, true);
    gfx_model_identity();
}
static M3 board_q(void) { return board_rot3(); }
static M3 bag_rotation(const Bag *b, bool *ok) {
    *ok = true;
    if (b->state == BAG_FLY) {
        float tilt = b->arc == ARC_KOBE ? 1 : 0.25f;
        return m3_euler_xyz(sinf((float)(b->t * b->tumble)) * .25f * tilt, (float)(b->release_yaw + b->spin * b->t),
                            cosf((float)(b->t * b->tumble * .7)) * .15f * tilt);
    }
    if (bag_on_deck(b) || b->state == BAG_HOLE) {
        M3 q = board_q(), y = m3_euler_xyz(0, (float)b->yaw, 0);
        return m3_mul(&q, &y);
    }
    return m3_euler_xyz(0, (float)b->yaw, 0);
}
static void draw_bags(const Game *g) {
    for (int i = 0; i < g->nbags; i++) {
        const Bag *b = &g->bags[i];
        if (b->state == BAG_IN || b->state == BAG_REMOVED) continue;
        bool ok; M3 r = bag_rotation(b, &ok);
        draw_bag_mesh((V3){(float)b->p.x, (float)b->p.y, (float)b->p.z}, &r, b->state == BAG_HOLE ? .53f : 1, b->team, b->side);
    }
}
static void draw_held(const Game *g, float dip) {
    if (!(g->phase == PH_AIM || g->phase == PH_CPU)) return;
    int t = game_thrower(g);
    double rx = game_release_x(g, t);
    double yaw = (game_is_human(g, t) ? base_yaw(0) : base_yaw(rx)) + g->aim;
    M3 r = m3_euler_xyz(.7f + dip * .35f, (float)-yaw, 0);
    V3 p = {(float)rx, (float)BYC_REL_Y - dip * .05f, (float)BYC_REL_Z + dip * .04f};
    /* held = TEAM_MATS[t][side=='slick' ? 'sticky' : 'slick'] on every face */
    gfx_model(p, &r, (V3){1, 1, 1});
    tex_bind(&tex_bag[t][g->side[t] == SIDE_SLICK ? 1 : 0]);
    draw_bag_faces(&r, (Rgba){255, 255, 255, 255}, false, false, true);
    gfx_model_identity();
}

/* ---------------- board deck with a real hole ---------------- */
static void deck_vertex(float x, float z, Rgba c) {
    V3 w = deck_world(x, 0, z);
    GX_Position3f32(w.x, w.y, w.z); GX_Color4u8(c.r, c.g, c.b, c.a);
    GX_TexCoord2f32(x / (float)BYC_W + .5f, 1 + z / (float)BYC_L);
}
static void deck_quad(float x0, float z0, float x1, float z1, Rgba c) {
    GX_Begin(GX_QUADS, GX_VTXFMT1, 4);
    deck_vertex(x0, z0, c); deck_vertex(x1, z0, c); deck_vertex(x1, z1, c); deck_vertex(x0, z1, c);
    GX_End();
}
static void draw_deck(void) {
    M3 r = board_rot3();
    Rgba c = shade((Rgba){255, 255, 255, 255}, m3_apply(&r, (V3){0, 1, 0}));
    const float W = (float)BYC_W, L = (float)BYC_L, hz = (float)byc_board.hole_local.z, R = (float)BYC_HOLE_R, sq = R + .03f;
    tex_bind(&tex_board);
    deck_quad(-W / 2, hz + sq, W / 2, 0, c);
    deck_quad(-W / 2, -L, W / 2, hz - sq, c);
    deck_quad(-W / 2, hz - sq, -sq, hz + sq, c);
    deck_quad(sq, hz - sq, W / 2, hz + sq, c);
    const int N = 32;
    GX_Begin(GX_QUADS, GX_VTXFMT1, N * 4);
    for (int i = 0; i < N; i++) {
        float a0 = 2 * (float)M_PI * i / N, a1 = 2 * (float)M_PI * (i + 1) / N;
        float c0 = cosf(a0), s0 = sinf(a0), c1 = cosf(a1), s1 = sinf(a1);
        float k0 = sq / fmaxf(fabsf(c0), fabsf(s0)), k1 = sq / fmaxf(fabsf(c1), fabsf(s1));
        deck_vertex(R * c0, hz + R * s0, c); deck_vertex(R * c1, hz + R * s1, c);
        deck_vertex(k1 * c1, hz + k1 * s1, c); deck_vertex(k0 * c0, hz + k0 * s0, c);
    }
    GX_End();
    /* hole wall */
    tex_bind(NULL);
    GX_Begin(GX_QUADS, GX_VTXFMT0, N * 4);
    for (int i = 0; i < N; i++) {
        float a0 = 2 * (float)M_PI * i / N, a1 = 2 * (float)M_PI * (i + 1) / N;
        V3 p[4] = {deck_world(R * cosf(a0), 0, hz + R * sinf(a0)), deck_world(R * cosf(a1), 0, hz + R * sinf(a1)),
                   deck_world(R * cosf(a1), -.02f, hz + R * sinf(a1)), deck_world(R * cosf(a0), -.02f, hz + R * sinf(a0))};
        for (int k = 0; k < 4; k++) { GX_Position3f32(p[k].x, p[k].y, p[k].z); GX_Color4u8(0x6b, 0x48, 0x2c, 255); }
    }
    GX_End();
}

static void draw_ground(float half, float reps) {
    Rgba c = shade((Rgba){255, 255, 255, 255}, (V3){0, 1, 0});
    tex_bind(&tex_grass);
    gfx_quad3_tex((V3){-half, 0, half}, (V3){half, 0, half}, (V3){half, 0, -half}, (V3){-half, 0, -half}, c, 0, 0, reps, reps);
}

static void textured_box(V3 c, V3 s, const M3 *rot, const Tex *t) {
    static const float N[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    static const float V[6][4][3] = {
        {{1, -1, 1}, {1, -1, -1}, {1, 1, -1}, {1, 1, 1}}, {{-1, -1, -1}, {-1, -1, 1}, {-1, 1, 1}, {-1, 1, -1}},
        {{-1, 1, 1}, {1, 1, 1}, {1, 1, -1}, {-1, 1, -1}}, {{-1, -1, -1}, {1, -1, -1}, {1, -1, 1}, {-1, -1, 1}},
        {{-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}}, {{1, -1, -1}, {-1, -1, -1}, {-1, 1, -1}, {1, 1, -1}}};
    static const float UV[4][2] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};
    tex_bind(t);
    GX_Begin(GX_QUADS, GX_VTXFMT1, 24);
    for (int f = 0; f < 6; f++) {
        V3 n = m3_apply(rot, (V3){N[f][0], N[f][1], N[f][2]});
        float len = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z); n.x /= len; n.y /= len; n.z /= len;
        Rgba col = shade((Rgba){255, 255, 255, 255}, n);
        for (int k = 0; k < 4; k++) {
            V3 l = m3_apply(rot, (V3){V[f][k][0] * s.x / 2, V[f][k][1] * s.y / 2, V[f][k][2] * s.z / 2});
            GX_Position3f32(c.x + l.x, c.y + l.y, c.z + l.z); GX_Color4u8(col.r, col.g, col.b, 255); GX_TexCoord2f32(UV[k][0], UV[k][1]);
        }
    }
    GX_End();
}

/* hay bales + pennant (index.html props) */
static V3 hay_world(V3 local, const M3 *r) {
    V3 l = m3_apply(r, (V3){local.x * .8f, local.y * .8f, local.z * .8f});
    return (V3){-2.7f + l.x, l.y, -9.6f + l.z};
}
static void draw_hay_and_pennant(const Game *g) {
    M3 r = m3_euler_xyz(0, .35f, 0), rs = r;
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) rs.m[i][j] *= .8f;
    static const float bales[3][3] = {{0, .225f, 0}, {.15f, .225f, -.55f}, {.05f, .675f, -.3f}};
    for (int i = 0; i < 3; i++) textured_box(hay_world((V3){bales[i][0], bales[i][1], bales[i][2]}, &r), (V3){1.0f, .45f, .5f}, &rs, &tex_hay);
    gfx_box(hay_world((V3){.1f, .9f + .25f, -.3f}, &r), (V3){.03f * .8f, .5f * .8f, .03f * .8f}, rgb(0x7a4b2a), &r);
    /* animatePennant(seconds): 17x7 grid in world space */
    V3 base = hay_world((V3){.1f + .015f, .9f + .48f, -.3f}, &r);
    const Wind *w = &g->wind;
    float strength = fminf(1, (float)w->mph / 12), extension = w->mph > 0 ? .45f + .55f * strength : 0;
    float dx = w->mph > 0 ? sinf((float)w->angle) : 1, dz = w->mph > 0 ? -cosf((float)w->angle) : 0;
    float freq = 2 + (float)w->mph * 1.1f, amp = w->mph > 0 ? .035f * (1 - .65f * strength) : 0;
    float sec = (float)scene_time;
    V3 pos[7][17];
    for (int row = 0; row < 7; row++) for (int col = 0; col < 17; col++) {
        float u = col / 16.0f, rr = row / 6.0f;
        float along = .32f * u * extension + .018f * u * (1 - extension);
        float ripple = sinf(sec * freq - u * 11) * amp * u + .012f * sinf(rr * (float)M_PI * 2) * u * (1 - extension);
        pos[row][col] = (V3){base.x + dx * along - dz * ripple, base.y - .2f * rr * (1 - .8f * u) - .30f * u * (1 - extension), base.z + dz * along + dx * ripple};
    }
    tex_bind(NULL);
    GX_Begin(GX_QUADS, GX_VTXFMT0, 6 * 16 * 4);
    for (int row = 0; row < 6; row++) for (int col = 0; col < 16; col++) {
        int rows[4] = {row, row, row + 1, row + 1}, cols[4] = {col, col + 1, col + 1, col};
        for (int k = 0; k < 4; k++) {
            float yorig = .1f - rows[k] * (.2f / 6);
            Rgba c = shade(rgb(fabsf(yorig) < .04f ? 0xfff4dc : 0xff6a4d), (V3){0, .3f, .95f});
            V3 p = pos[rows[k]][cols[k]];
            GX_Position3f32(p.x, p.y, p.z); GX_Color4u8(c.r, c.g, c.b, 255);
        }
    }
    GX_End();
}

/* pixel dog + DogCelebration (hops / spin / bow) */
static void dog_box(float w, float h, float d, unsigned col, float x, float y, float z, const M3 *m, V3 off) {
    V3 o = m3_apply(m, (V3){x, y, z});
    gfx_box((V3){off.x + o.x, off.y + o.y, off.z + o.z}, (V3){w, h, d}, rgb(col), m);
}
static void draw_dog(const Game *g) {
    V3 gp = {2.3f, 0, -8.7f}; float rx = 0, ry = 2.6f, rz = 0, sy = .85f;
    float head_y = .55f + fabsf(sinf((float)scene_time * 1.5f)) * .008f, head_rx = 0, head_rz = 0;
    float tail_y = sinf((float)scene_time * (g->phase == PH_FLYING ? 18 : 7)) * .6f;
    if (dog_party.active) {
        float t = dog_party.elapsed / 2.2f, e = sinf((float)M_PI * t);
        tail_y = sinf(dog_party.elapsed * 30) * 1.1f;
        if (dog_party.index == 0) { gp.y = fabsf(sinf(t * (float)M_PI * 3)) * .34f * e; rz = sinf(t * (float)M_PI * 6) * .08f * e; head_rx = -.18f * e; }
        else if (dog_party.index == 1) { float turn = t * t * (3 - 2 * t); ry += turn * (float)M_PI * 2; gp.y = e * .15f; head_rz = sinf(t * (float)M_PI * 4) * .16f * e; }
        else { float bounce = sinf(t * (float)M_PI * 4); rx = -.22f * e; gp.y = fmaxf(0, bounce) * .12f * e; head_y -= .08f * e; head_rz = sinf(t * (float)M_PI * 6) * .22f * e; sy *= 1 - .1f * e; }
    }
    float head_ry = dog_head_y_rot;
    if (party.toss && !dog_party.active) {   /* party.mjs updateCook: watch the burger, wag, catch */
        head_ry = (float)party_dog_head_yaw((Vec3){gp.x, gp.y, gp.z}, ry);
        head_rz = -sinf((float)(party.toss_progress * M_PI)) * .18f;
        tail_y = sinf((float)party.time * 24);
    }
    M3 R = m3_euler_xyz(rx, ry, rz), S = {{{.85f, 0, 0}, {0, sy, 0}, {0, 0, .85f}}}, M = m3_mul(&R, &S);
    const unsigned fur = 0xe89a45, cream = 0xfff1d6, dark = 0x2b1a2e, pink = 0xff8fb6;
#define DOGBOX(bw, bh, bd, col, bx, by, bz, MM, OFF) dog_box(bw, bh, bd, col, bx, by, bz, &MM, OFF)
    DOGBOX(.34f, .3f, .5f, fur, 0, .3f, .05f, M, gp);
    DOGBOX(.3f, .22f, .12f, cream, 0, .34f, -.22f, M, gp);
    for (int s = -1; s <= 1; s += 2) { DOGBOX(.08f, .2f, .08f, fur, s * .1f, .1f, -.15f, M, gp); DOGBOX(.09f, .04f, .1f, cream, s * .1f, .02f, -.18f, M, gp); }
    DOGBOX(.14f, .12f, .3f, fur, -.18f, .08f, .15f, M, gp); DOGBOX(.14f, .12f, .3f, fur, .18f, .08f, .15f, M, gp);
    V3 hp = m3_apply(&M, (V3){0, head_y, -.15f}); hp = (V3){gp.x + hp.x, gp.y + hp.y, gp.z + hp.z};
    M3 HR = m3_euler_xyz(head_rx, head_ry, head_rz), H = m3_mul(&M, &HR);
    { V3 m = m3_apply(&H, (V3){0, -.06f, -.22f}); dog_mouth = (V3){hp.x + m.x, hp.y + m.y, hp.z + m.z}; }
    DOGBOX(.3f, .26f, .26f, fur, 0, 0, 0, H, hp); DOGBOX(.16f, .1f, .12f, cream, 0, -.05f, -.17f, H, hp);
    DOGBOX(.06f, .05f, .04f, dark, 0, -.01f, -.24f, H, hp);
    DOGBOX(.05f, .05f, .02f, dark, -.08f, .05f, -.135f, H, hp); DOGBOX(.05f, .05f, .02f, dark, .08f, .05f, -.135f, H, hp);
    DOGBOX(.08f, .1f, .06f, fur, -.1f, .17f, 0, H, hp); DOGBOX(.08f, .1f, .06f, fur, .1f, .17f, 0, H, hp);
    DOGBOX(.04f, .06f, .02f, pink, 0, -.12f, -.2f, H, hp);
    V3 tp = m3_apply(&M, (V3){0, .35f, .3f}); tp = (V3){gp.x + tp.x, gp.y + tp.y, gp.z + tp.z};
    M3 TR = m3_euler_xyz(0, tail_y, 0), T = m3_mul(&M, &TR), TB = m3_euler_xyz(-.8f, 0, 0), T2 = m3_mul(&T, &TB);
    V3 to = m3_apply(&T, (V3){0, .06f, .08f});
    gfx_box((V3){tp.x + to.x, tp.y + to.y, tp.z + to.z}, (V3){.08f, .08f, .22f}, rgb(fur), &T2);
#undef DOGBOX
}

static void draw_particles(void) {
    for (int i = 0; i < 70; i++) if (confetti[i].life > 0) gfx_box_unlit(confetti[i].p, (V3){.04f, .04f, .04f}, confetti[i].c);
    gfx_blend(true); gfx_depth(true, false);
    for (int i = 0; i < 14; i++) if (dust[i].life > 0) gfx_box_unlit(dust[i].p, (V3){.04f, .04f, .04f}, rgba(0xe7cfaa, (u8)(.7f * dust[i].life / .4f * 255)));
    for (int i = 0; i < 5; i++) {
        float t = smoke_t[i], s = .16f * (.6f + t * 1.4f);
        gfx_box_unlit((V3){2.75f + sinf(t * 6 + 1) * .08f, .74f + t * .8f, -8.35f + t * .15f}, (V3){s, s, s}, rgba(0xffffff, (u8)(.55f * (1 - t) * 255)));
    }
    gfx_blend(false); gfx_depth(true, true);
}
static void draw_trail(const Game *g) {
    if (g->phase != PH_FLYING) return;
    gfx_blend(true); gfx_depth(true, false);
    for (int i = 0; i < 3; i++) if (trail[i].life > 0) {
        gfx_model(trail[i].p, &trail[i].r, (V3){1, 1, 1});
        tex_bind(NULL);
        Rgba c = rgba(trail[i].team ? 0x6b93ff : 0xff6b5e, (u8)(.24f * trail[i].life / .18f * 255));
        gfx_box_unlit((V3){0, 0, 0}, (V3){(float)BYC_BAG, (float)BYC_BAG_H * 2, (float)BYC_BAG}, c);
        gfx_model_identity();
    }
    gfx_blend(false); gfx_depth(true, true);
}

/* wind-streaks.js: 24 faint two-dash gusts (LineSegments, #e5efdc, no depth write) */
static void draw_wind_streaks(void) {
    if (!streaks.visible || !streaks.active) return;
    Rgba c = rgba(0xe5efdc, (u8)(streaks.opacity * 255 + .5));
    tex_bind(NULL);
    gfx_blend(true); gfx_depth(true, false);
    GX_SetLineWidth(8, GX_TO_ZERO);
    GX_Begin(GX_LINES, GX_VTXFMT0, (u16)(streaks.active * STREAK_VERTS));
    for (int i = 0; i < streaks.active * STREAK_VERTS; i++) {
        GX_Position3f32((float)streaks.verts[i].x, (float)streaks.verts[i].y, (float)streaks.verts[i].z);
        GX_Color4u8(c.r, c.g, c.b, c.a);
    }
    GX_End();
    gfx_blend(false); gfx_depth(true, true);
}

void scene_draw(const Game *g, float held_dip) {
    gfx_perspective((float)VIEW_FOV_DEG, gfx_aspect, .1f, 200);   /* effective landscape FOV 18 (resize(): 18 + 6*portrait) */
    gfx_look((V3){(float)VIEW_EYE_X, (float)VIEW_EYE_Y, (float)VIEW_EYE_Z}, (V3){(float)VIEW_AT_X, (float)VIEW_AT_Y, (float)VIEW_AT_Z});
    gfx_fog(true);
    gfx_depth(true, true); gfx_blend(false);
    draw_ground(30, 24);
    dl_call(&yard);
    draw_deck();
    draw_hay_and_pennant(g);
    draw_dog(g);
    party_draw(&party, dog_mouth);
    draw_bags(g);
    draw_held(g, held_dip);
    draw_particles();
    draw_trail(g);
    draw_wind_streaks();
}

void scene_draw_board_view(const Game *g, int x, int y, int w, int h) {
    /* BoardView: second viewport, clear colour + depth inside the inset, then board/bags/ground. */
    gfx_viewport(x, y, w, h);
    gfx_begin_2d();
    gfx_depth(true, true);
    GX_SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
    gfx_blend(false);
    tex_bind(NULL);
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
    float zf = -999.9f;
    GX_Position3f32(0, 0, zf); GX_Color4u8(0, 0, 0, 255); GX_Position3f32(640, 0, zf); GX_Color4u8(0, 0, 0, 255);
    GX_Position3f32(640, 480, zf); GX_Color4u8(0, 0, 0, 255); GX_Position3f32(0, 480, zf); GX_Color4u8(0, 0, 0, 255);
    GX_End();
    gfx_perspective(30, (float)w / h, .05f, 30);
    gfx_look((V3){0, 2.5f, (float)BYC_BOARD_Z + .75f}, (V3){0, .16f, (float)BYC_BOARD_Z - .55f});
    gfx_fog(true); gfx_depth(true, true);
    draw_ground(30, 24);
    draw_deck();
    tex_bind(NULL);
    M3 r = board_rot3();
    const float W = (float)BYC_W, L = (float)BYC_L, sk = .09f;
    gfx_box(deck_world(-W / 2 + .0125f, -sk / 2 - .02f, -L / 2), (V3){.025f, sk, L}, rgb(0xd9a066), &r);
    gfx_box(deck_world(W / 2 - .0125f, -sk / 2 - .02f, -L / 2), (V3){.025f, sk, L}, rgb(0xd9a066), &r);
    gfx_box(deck_world(0, -sk / 2 - .02f, -.0125f), (V3){W, sk, .025f}, rgb(0xd9a066), &r);
    gfx_box(deck_world(0, -.1f, -L / 2), (V3){W - .05f, .002f, L - .05f}, rgb(0x1c120c), &r);
    draw_bags(g);
    gfx_full_viewport();
    /* restore the main camera so later projections (shot arrow) use it */
    gfx_perspective((float)VIEW_FOV_DEG, gfx_aspect, .1f, 200);
    gfx_look((V3){(float)VIEW_EYE_X, (float)VIEW_EYE_Y, (float)VIEW_EYE_Z}, (V3){(float)VIEW_AT_X, (float)VIEW_AT_Y, (float)VIEW_AT_Z});
}

bool scene_bag_screen(const Game *g, float *sx, float *sy) {
    int t = game_thrower(g);
    V3 p = {(float)game_release_x(g, t), (float)BYC_REL_Y, (float)BYC_REL_Z};
    return gfx_project(p, sx, sy);
}
