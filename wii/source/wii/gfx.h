/* GX helpers: NTSC-only video, two vertex formats (colour; colour + texture), RGB5A3 textures,
 * CPU-lit voxel boxes (static scenery is recorded once into display lists), 2D quads and the
 * 8x16 libogc console font for HUD text. Raw GX, no GRRLIB (SPEC §1.1). */
#ifndef BYC_GFX_H
#define BYC_GFX_H

#include <gccore.h>
#include <stdbool.h>

typedef struct { float x, y, z; } V3;
typedef struct { float m[3][3]; } M3;
typedef struct { u8 r, g, b, a; } Rgba;

extern GXRModeObj *gfx_mode;
extern float gfx_aspect;      /* 4:3 or 16:9 (console setting) */
/* 2D virtual screen: gfx_ui_w x 480 square units (640 at 4:3, 853.3 at 16:9). The EFB stays
 * 640 wide; gfx_ui_to_efb converts a 2D x or width to EFB pixels (for viewports). */
extern float gfx_ui_w;
float gfx_ui_to_efb(float x);
extern bool gfx_widescreen, gfx_progressive;

void gfx_init(void);
void gfx_begin_frame(void);
void gfx_end_frame(void);

/* Matrices / passes */
void gfx_perspective(float fovy, float aspect, float znear, float zfar);
void gfx_look(V3 eye, V3 target);
void gfx_model(V3 pos, const M3 *rot, V3 scale);   /* rot may be NULL */
void gfx_model_identity(void);
void gfx_viewport(int x, int y, int w, int h);
void gfx_full_viewport(void);
void gfx_fog(bool on);
void gfx_begin_2d(void);
void gfx_depth(bool test, bool write);
void gfx_blend(bool on);
bool gfx_project(V3 world, float *sx, float *sy);  /* using the last perspective + look */

/* Textures */
typedef struct { GXTexObj obj; void *data; int w, h; } Tex;
void tex_create(Tex *t, int w, int h, bool repeat);
void tex_set(Tex *t, int x, int y, Rgba c);
void tex_fill(Tex *t, int x, int y, int w, int h, Rgba c);
void tex_upload(Tex *t);
void tex_bind(const Tex *t);   /* NULL = untextured (vertex colour only) */

/* Colour */
Rgba rgb(unsigned hex);
Rgba rgba(unsigned hex, u8 a);
Rgba shade(Rgba c, V3 normal);     /* sun + hemisphere approximation of the 1.0.3 lights */

/* 3D primitives (in the current model space) */
M3 m3_identity(void);
M3 m3_euler_xyz(float x, float y, float z);   /* Three.js default Euler order */
M3 m3_mul(const M3 *a, const M3 *b);
V3 m3_apply(const M3 *m, V3 v);
void gfx_box(V3 center, V3 size, Rgba color, const M3 *rot);   /* lit, world-space rotation */
void gfx_box_unlit(V3 center, V3 size, Rgba color);
/* A zero inner radius gives a solid cylinder. A positive one opens the centre for a rim. */
void gfx_cylinder(V3 center, float radius, float inner_radius, float height, Rgba color, const M3 *rot);
void gfx_sphere(V3 center, float radius, Rgba color);
void gfx_quad3(V3 a, V3 b, V3 c, V3 d, Rgba color);
void gfx_quad3_tex(V3 a, V3 b, V3 c, V3 d, Rgba color, float s0, float t0, float s1, float t1);

/* Display lists */
typedef struct { void *buf; u32 size, len; } DList;
void dl_begin(DList *dl, u32 size);
void dl_end(DList *dl);
void dl_call(const DList *dl);

/* 2D (640x480 virtual screen) */
void gfx_rect(float x, float y, float w, float h, Rgba c);
void gfx_frame(float x, float y, float w, float h, float t, Rgba c);
void gfx_tri2(float x0, float y0, float x1, float y1, float x2, float y2, Rgba c);
/* Text: 8x16 glyphs scaled by s. Bytes >= 0x80 are CP437 glyphs (0xFA middle dot, 0x10/0x11 arrows). */
float gfx_text(float x, float y, float s, Rgba c, const char *text);
float gfx_text_width(const char *text, float s);
void gfx_text_center(float cx, float y, float s, Rgba c, const char *text);
void gfx_text_shadow(float x, float y, float s, Rgba c, const char *text);
void gfx_text_center_shadow(float cx, float y, float s, Rgba c, const char *text);

#endif
