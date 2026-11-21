#include "gfx.h"
#include <malloc.h>
#include <math.h>
#include <string.h>

#define FIFO_SIZE (256 * 1024)

GXRModeObj *gfx_mode;
float gfx_aspect = 4.0f / 3.0f;
float gfx_ui_w = 640.0f;
bool gfx_widescreen, gfx_progressive;

static void *xfb[2];
static int fb;
static Mtx view;
static Mtx44 proj;
static bool textured;
extern const u8 console_font_8x16[];
static Tex font;

/* ---------------- video ---------------- */
void gfx_init(void) {
    VIDEO_Init();
    /* NTSC / 60 Hz only (SPEC §1.5): no PAL or EURGB60 branch, whatever the console region. */
    gfx_progressive = CONF_GetProgressiveScan() > 0 && VIDEO_HaveComponentCable();
    gfx_mode = gfx_progressive ? &TVNtsc480Prog : &TVNtsc480IntDf;
    gfx_widescreen = CONF_GetAspectRatio() == CONF_ASPECT_16_9;
    if (gfx_widescreen) {
        gfx_mode->viWidth = 678;
        gfx_mode->viXOrigin = (VI_MAX_WIDTH_NTSC - 678) / 2;
        gfx_aspect = 16.0f / 9.0f;
    }
    gfx_ui_w = 480.0f * gfx_aspect;
    VIDEO_Configure(gfx_mode);
    for (int i = 0; i < 2; i++) {
        xfb[i] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(gfx_mode));
        VIDEO_ClearFrameBuffer(gfx_mode, xfb[i], COLOR_BLACK);
    }
    VIDEO_SetNextFramebuffer(xfb[0]);
    VIDEO_SetBlack(false);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (gfx_mode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    void *fifo = memalign(32, FIFO_SIZE);
    memset(fifo, 0, FIFO_SIZE);
    GX_Init(fifo, FIFO_SIZE);
    /* Clear to the sky sphere's horizon colour (backyard.js skyBottom #f1cba2; the sky has no fog). */
    GXColor sky = {0xf1, 0xcb, 0xa2, 0xff};
    GX_SetCopyClear(sky, GX_MAX_Z24);
    gfx_full_viewport();
    f32 yscale = GX_GetYScaleFactor(gfx_mode->efbHeight, gfx_mode->xfbHeight);
    u32 xfb_height = GX_SetDispCopyYScale(yscale);
    GX_SetDispCopySrc(0, 0, gfx_mode->fbWidth, gfx_mode->efbHeight);
    GX_SetDispCopyDst(gfx_mode->fbWidth, xfb_height);
    GX_SetCopyFilter(gfx_mode->aa, gfx_mode->sample_pattern, GX_TRUE, gfx_mode->vfilter);
    GX_SetFieldMode(gfx_mode->field_rendering, gfx_mode->viHeight == 2 * gfx_mode->xfbHeight ? GX_ENABLE : GX_DISABLE);
    GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
    GX_SetCullMode(GX_CULL_NONE);
    GX_CopyDisp(xfb[fb], GX_TRUE);
    GX_SetDispCopyGamma(GX_GM_1_0);

    GX_ClearVtxDesc();
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GX_SetNumChans(1);
    GX_SetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHTNULL, GX_DF_NONE, GX_AF_NONE);
    GX_SetNumTevStages(1);
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GX_SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GX_SetZCompLoc(GX_FALSE);   /* z after texturing so the alpha test discards glyph holes */
    GX_SetColorUpdate(GX_TRUE);
    GX_SetAlphaUpdate(GX_FALSE);
    textured = true; tex_bind(NULL);
    gfx_blend(false);
    gfx_depth(true, true);

    /* Font atlas: 16x16 grid of the libogc 8x16 console font (CP437), white with alpha. */
    tex_create(&font, 128, 256, false);
    for (int c = 0; c < 256; c++)
        for (int row = 0; row < 16; row++) {
            u8 bits = console_font_8x16[c * 16 + row];
            for (int col = 0; col < 8; col++)
                tex_set(&font, (c % 16) * 8 + col, (c / 16) * 16 + row, (Rgba){255, 255, 255, (bits & (0x80 >> col)) ? 255 : 0});
        }
    tex_upload(&font);
}

void gfx_begin_frame(void) {
    gfx_full_viewport();
}
void gfx_end_frame(void) {
    GX_DrawDone();
    gfx_depth(true, true);
    GX_SetColorUpdate(GX_TRUE);
    GX_CopyDisp(xfb[fb], GX_TRUE);
    GX_Flush();
    VIDEO_SetNextFramebuffer(xfb[fb]);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    fb ^= 1;
}

/* ---------------- state ---------------- */
void gfx_viewport(int x, int y, int w, int h) {
    GX_SetViewport((f32)x, (f32)y, (f32)w, (f32)h, 0, 1);
    GX_SetScissor((u32)x, (u32)y, (u32)w, (u32)h);
}
void gfx_full_viewport(void) { gfx_viewport(0, 0, gfx_mode->fbWidth, gfx_mode->efbHeight); }
void gfx_depth(bool test, bool write) { GX_SetZMode(test ? GX_TRUE : GX_FALSE, GX_LEQUAL, write ? GX_TRUE : GX_FALSE); }
void gfx_blend(bool on) {
    if (on) GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    else GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR);
}
void gfx_fog(bool on) {
    GXColor c = {0xc4, 0xd3, 0xa7, 0xff};
    if (on) GX_SetFog(GX_FOG_LIN, 26.0f, 75.0f, 0.1f, 200.0f, c);
    else GX_SetFog(GX_FOG_NONE, 0, 0, 0, 0, c);
}
void gfx_perspective(float fovy, float aspect, float znear, float zfar) {
    guPerspective(proj, fovy, aspect, znear, zfar);
    GX_LoadProjectionMtx(proj, GX_PERSPECTIVE);
}
void gfx_look(V3 eye, V3 target) {
    guVector e = {eye.x, eye.y, eye.z}, up = {0, 1, 0}, t = {target.x, target.y, target.z};
    guLookAt(view, &e, &up, &t);
    gfx_model_identity();
}
void gfx_model_identity(void) {
    GX_LoadPosMtxImm(view, GX_PNMTX0);
    GX_SetCurrentMtx(GX_PNMTX0);
}
void gfx_model(V3 p, const M3 *r, V3 s) {
    Mtx m, mv;
    M3 id = m3_identity();
    if (!r) r = &id;
    for (int i = 0; i < 3; i++) {
        m[i][0] = r->m[i][0] * s.x; m[i][1] = r->m[i][1] * s.y; m[i][2] = r->m[i][2] * s.z;
    }
    m[0][3] = p.x; m[1][3] = p.y; m[2][3] = p.z;
    guMtxConcat(view, m, mv);
    GX_LoadPosMtxImm(mv, GX_PNMTX0);
    GX_SetCurrentMtx(GX_PNMTX0);
}
void gfx_begin_2d(void) {
    Mtx44 o; Mtx id;
    guOrtho(o, 0, 480, 0, gfx_ui_w, 0, 1000);
    GX_LoadProjectionMtx(o, GX_ORTHOGRAPHIC);
    guMtxIdentity(id);
    GX_LoadPosMtxImm(id, GX_PNMTX0);
    GX_SetCurrentMtx(GX_PNMTX0);
    gfx_fog(false);
    gfx_depth(false, false);
    gfx_blend(true);
}
bool gfx_project(V3 w, float *sx, float *sy) {
    float ex = view[0][0] * w.x + view[0][1] * w.y + view[0][2] * w.z + view[0][3];
    float ey = view[1][0] * w.x + view[1][1] * w.y + view[1][2] * w.z + view[1][3];
    float ez = view[2][0] * w.x + view[2][1] * w.y + view[2][2] * w.z + view[2][3];
    float cx = proj[0][0] * ex + proj[0][1] * ey + proj[0][2] * ez + proj[0][3];
    float cy = proj[1][0] * ex + proj[1][1] * ey + proj[1][2] * ez + proj[1][3];
    float cw = -ez;
    if (cw <= 0.001f) return false;
    *sx = (cx / cw + 1) * gfx_ui_w * 0.5f;
    *sy = (1 - cy / cw) * 240.0f;
    return true;
}

float gfx_ui_to_efb(float x) { return x * (float)gfx_mode->fbWidth / gfx_ui_w; }

/* ---------------- textures (RGB5A3, 4x4 texel tiles) ---------------- */
void tex_create(Tex *t, int w, int h, bool repeat) {
    t->w = w; t->h = h;
    t->data = memalign(32, (size_t)(w * h * 2));
    memset(t->data, 0, (size_t)(w * h * 2));
    GX_InitTexObj(&t->obj, t->data, (u16)w, (u16)h, GX_TF_RGB5A3, repeat ? GX_REPEAT : GX_CLAMP, repeat ? GX_REPEAT : GX_CLAMP, GX_FALSE);
    GX_InitTexObjLOD(&t->obj, GX_NEAR, GX_NEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
}
void tex_set(Tex *t, int x, int y, Rgba c) {
    if (x < 0 || y < 0 || x >= t->w || y >= t->h) return;
    u16 v;
    if (c.a >= 0xE0) v = (u16)(0x8000 | ((c.r >> 3) << 10) | ((c.g >> 3) << 5) | (c.b >> 3));
    else v = (u16)(((c.a >> 5) << 12) | ((c.r >> 4) << 8) | ((c.g >> 4) << 4) | (c.b >> 4));
    int tiles_x = t->w / 4;
    int tile = (y / 4) * tiles_x + (x / 4);
    int idx = tile * 16 + (y % 4) * 4 + (x % 4);
    ((u16 *)t->data)[idx] = v;
}
void tex_fill(Tex *t, int x, int y, int w, int h, Rgba c) {
    for (int j = y; j < y + h; j++) for (int i = x; i < x + w; i++) tex_set(t, i, j, c);
}
void tex_upload(Tex *t) {
    DCFlushRange(t->data, (u32)(t->w * t->h * 2));
    GX_InvalidateTexAll();
}
void tex_bind(const Tex *t) {
    if (t) {
        GX_LoadTexObj((GXTexObj *)&t->obj, GX_TEXMAP0);
        if (!textured) {
            GX_ClearVtxDesc();
            GX_SetVtxDesc(GX_VA_POS, GX_DIRECT); GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT); GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
            GX_SetNumTexGens(1);
            GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
            GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
            textured = true;
        }
    } else if (textured) {
        GX_ClearVtxDesc();
        GX_SetVtxDesc(GX_VA_POS, GX_DIRECT); GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
        GX_SetNumTexGens(0);
        GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
        GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
        textured = false;
    }
}

/* ---------------- colour + lighting ---------------- */
Rgba rgb(unsigned h) { return (Rgba){(u8)(h >> 16), (u8)(h >> 8), (u8)h, 255}; }
Rgba rgba(unsigned h, u8 a) { Rgba c = rgb(h); c.a = a; return c; }
static u8 sat(float v) { return v <= 0 ? 0 : v >= 255 ? 255 : (u8)v; }
Rgba shade(Rgba c, V3 n) {
    /* Sun #ffe0b0 from (-7,12,5) toward (0,0,-5); hemisphere sky #d6ecff / ground #7a8f3a.
     * A display-referred approximation tuned by eye; no shadow maps (SPEC §2.5). */
    static const V3 L = {-0.4094f, 0.7018f, 0.5848f};
    float sun = n.x * L.x + n.y * L.y + n.z * L.z;
    if (sun < 0) sun = 0;
    float hemi = 0.5f * n.y + 0.5f;
    float base = 0.50f + 0.18f * hemi;
    float r = base * (0.80f + 0.20f * hemi) + 0.52f * sun;
    float g = base * (0.86f + 0.14f * hemi) + 0.46f * sun;
    float b = base * (0.78f + 0.22f * hemi) + 0.36f * sun;
    return (Rgba){sat(c.r * r), sat(c.g * g), sat(c.b * b), c.a};
}

/* ---------------- 3D primitives ---------------- */
M3 m3_identity(void) { M3 m = {{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}}; return m; }
M3 m3_mul(const M3 *a, const M3 *b) {
    M3 r;
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++)
        r.m[i][j] = a->m[i][0] * b->m[0][j] + a->m[i][1] * b->m[1][j] + a->m[i][2] * b->m[2][j];
    return r;
}
M3 m3_euler_xyz(float x, float y, float z) {
    float a = cosf(x), b = sinf(x), c = cosf(y), d = sinf(y), e = cosf(z), f = sinf(z);
    float ae = a * e, af = a * f, be = b * e, bf = b * f;
    M3 m = {{{c * e, -c * f, d}, {af + be * d, ae - bf * d, -b * c}, {bf - ae * d, be + af * d, a * c}}};
    return m;
}
V3 m3_apply(const M3 *m, V3 v) {
    return (V3){m->m[0][0] * v.x + m->m[0][1] * v.y + m->m[0][2] * v.z,
                m->m[1][0] * v.x + m->m[1][1] * v.y + m->m[1][2] * v.z,
                m->m[2][0] * v.x + m->m[2][1] * v.y + m->m[2][2] * v.z};
}
static void vtx(V3 p, Rgba c) { GX_Position3f32(p.x, p.y, p.z); GX_Color4u8(c.r, c.g, c.b, c.a); }
static void vtxt(V3 p, Rgba c, float s, float t) { GX_Position3f32(p.x, p.y, p.z); GX_Color4u8(c.r, c.g, c.b, c.a); GX_TexCoord2f32(s, t); }

static const float FACE_N[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
static const float FACE_V[6][4][3] = {
    {{1, -1, -1}, {1, 1, -1}, {1, 1, 1}, {1, -1, 1}},
    {{-1, -1, 1}, {-1, 1, 1}, {-1, 1, -1}, {-1, -1, -1}},
    {{-1, 1, -1}, {-1, 1, 1}, {1, 1, 1}, {1, 1, -1}},
    {{-1, -1, 1}, {-1, -1, -1}, {1, -1, -1}, {1, -1, 1}},
    {{-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}},
    {{1, -1, -1}, {-1, -1, -1}, {-1, 1, -1}, {1, 1, -1}},
};
static void box_impl(V3 c, V3 s, Rgba color, const M3 *rot, bool lit) {
    tex_bind(NULL);
    GX_Begin(GX_QUADS, GX_VTXFMT0, 24);
    for (int f = 0; f < 6; f++) {
        V3 n = {FACE_N[f][0], FACE_N[f][1], FACE_N[f][2]};
        if (rot) {
            n = m3_apply(rot, n);
            float len = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z);
            if (len > 1e-6f) { n.x /= len; n.y /= len; n.z /= len; }
        }
        Rgba fc = lit ? shade(color, n) : color;
        for (int k = 0; k < 4; k++) {
            V3 l = {FACE_V[f][k][0] * s.x * 0.5f, FACE_V[f][k][1] * s.y * 0.5f, FACE_V[f][k][2] * s.z * 0.5f};
            if (rot) l = m3_apply(rot, l);
            vtx((V3){c.x + l.x, c.y + l.y, c.z + l.z}, fc);
        }
    }
    GX_End();
}
void gfx_box(V3 c, V3 s, Rgba color, const M3 *rot) { box_impl(c, s, color, rot, true); }
void gfx_box_unlit(V3 c, V3 s, Rgba color) { box_impl(c, s, color, NULL, false); }
void gfx_quad3(V3 a, V3 b, V3 c, V3 d, Rgba color) {
    tex_bind(NULL);
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4); vtx(a, color); vtx(b, color); vtx(c, color); vtx(d, color); GX_End();
}
void gfx_quad3_tex(V3 a, V3 b, V3 c, V3 d, Rgba color, float s0, float t0, float s1, float t1) {
    GX_Begin(GX_QUADS, GX_VTXFMT1, 4);
    vtxt(a, color, s0, t1); vtxt(b, color, s1, t1); vtxt(c, color, s1, t0); vtxt(d, color, s0, t0);
    GX_End();
}

/* ---------------- display lists ---------------- */
void dl_begin(DList *dl, u32 size) {
    size = (size + 31) & ~31u;
    dl->buf = memalign(32, size); dl->size = size; dl->len = 0;
    memset(dl->buf, 0, size);
    DCInvalidateRange(dl->buf, size);
    /* Lists hold untextured VTXFMT0 primitives only: vertex descriptors (CP state) are set
     * outside the list, before recording and before every call. */
    tex_bind(NULL);
    GX_BeginDispList(dl->buf, size);
}
void dl_end(DList *dl) { dl->len = GX_EndDispList(); }
void dl_call(const DList *dl) { tex_bind(NULL); if (dl->len) GX_CallDispList(dl->buf, dl->len); }

/* ---------------- 2D ---------------- */
void gfx_rect(float x, float y, float w, float h, Rgba c) {
    tex_bind(NULL);
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
    vtx((V3){x, y, -1}, c); vtx((V3){x + w, y, -1}, c); vtx((V3){x + w, y + h, -1}, c); vtx((V3){x, y + h, -1}, c);
    GX_End();
}
void gfx_frame(float x, float y, float w, float h, float t, Rgba c) {
    gfx_rect(x, y, w, t, c); gfx_rect(x, y + h - t, w, t, c); gfx_rect(x, y, t, h, c); gfx_rect(x + w - t, y, t, h, c);
}
void gfx_tri2(float x0, float y0, float x1, float y1, float x2, float y2, Rgba c) {
    tex_bind(NULL);
    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);
    vtx((V3){x0, y0, -1}, c); vtx((V3){x1, y1, -1}, c); vtx((V3){x2, y2, -1}, c);
    GX_End();
}
float gfx_text_width(const char *s, float scale) { return (float)strlen(s) * 8 * scale; }
float gfx_text(float x, float y, float s, Rgba c, const char *text) {
    size_t n = strlen(text);
    if (!n) return x;
    tex_bind(&font);
    GX_Begin(GX_QUADS, GX_VTXFMT1, (u16)(n * 4));
    for (size_t i = 0; i < n; i++) {
        u8 ch = (u8)text[i];
        float s0 = (ch % 16) * 8 / 128.0f, t0 = (ch / 16) * 16 / 256.0f, s1 = s0 + 8 / 128.0f, t1 = t0 + 16 / 256.0f;
        float x0 = x + i * 8 * s, x1 = x0 + 8 * s, y1 = y + 16 * s;
        vtxt((V3){x0, y, -1}, c, s0, t0); vtxt((V3){x1, y, -1}, c, s1, t0);
        vtxt((V3){x1, y1, -1}, c, s1, t1); vtxt((V3){x0, y1, -1}, c, s0, t1);
    }
    GX_End();
    return x + n * 8 * s;
}
void gfx_text_center(float cx, float y, float s, Rgba c, const char *t) { gfx_text(cx - gfx_text_width(t, s) / 2, y, s, c, t); }
void gfx_text_shadow(float x, float y, float s, Rgba c, const char *t) {
    gfx_text(x + s, y + s, s, (Rgba){0x17, 0x2c, 0x32, c.a}, t);
    gfx_text(x, y, s, c, t);
}
void gfx_text_center_shadow(float cx, float y, float s, Rgba c, const char *t) { gfx_text_shadow(cx - gfx_text_width(t, s) / 2, y, s, c, t); }
