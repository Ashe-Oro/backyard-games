/* Renderer for the 1.0.3 party NPCs (arcade/party.mjs): keg + pump + hose + picnic tap, the keg
 * guest and friend (with the keg-stand gag), the beer pong table with its scripted match, the
 * grill cook flipping food and tossing a burger to the dog, and the perimeter cacti. All motion
 * comes from source/core/party.c; this file only draws the props and voxel characters. */
#include "party_draw.h"
#include <math.h>

typedef struct { M3 r; V3 t; float sy; } Xf;   /* sy: non-uniform y scale of this group's own children */

static V3 add3(V3 a, V3 b) { return (V3){a.x + b.x, a.y + b.y, a.z + b.z}; }
static V3 v3f(Vec3 v) { return (V3){(float)v.x, (float)v.y, (float)v.z}; }
static Xf xf_root(V3 t, M3 r) { Xf x = {r, t, 1}; return x; }
static Xf xf_child(const Xf *p, V3 pos, M3 rot) {
    pos.y *= p->sy;
    Xf c = {m3_mul(&p->r, &rot), add3(p->t, m3_apply(&p->r, pos)), 1};
    return c;
}
static void cube(const Xf *p, float w, float h, float d, unsigned col, float x, float y, float z) {
    V3 c = add3(p->t, m3_apply(&p->r, (V3){x, y * p->sy, z}));
    gfx_box(c, (V3){w, h * p->sy, d}, rgb(col), &p->r);
}
static void cylinder(const Xf *p, float radius, float inner, float h, unsigned col, float y, float z) {
    V3 c = add3(p->t, m3_apply(&p->r, (V3){0, y * p->sy, z}));
    gfx_cylinder(c, radius, inner, h * p->sy, rgb(col), &p->r);
}
static void keg_cup(const Xf *p, float y, float z, float lip_h) {
    cylinder(p, .045f, 0, .11f, 0xd15a49, y, z);
    cylinder(p, .0475f, .039f, lip_h, 0xfff1d1, y + .06f, z);
}

/* person(): torso, head, hair, eyes, legs, free arm group, other arm, optional cup in hand.
 * pong: the beer pong players' elbow joint (forearm hinged at -.15 in the arm). */
typedef struct {
    unsigned shirt, skin, hair;
    float handedness, arm_x, arm_z, arm_sy, elbow;
    bool cup, pong;
} PersonLook;
static void person(const Xf *g, const PersonLook *l) {
    cube(g, .27f, .34f, .17f, l->shirt, 0, .66f, 0);
    cube(g, .24f, .23f, .23f, l->skin, 0, .97f, 0);
    cube(g, .25f, .075f, .24f, l->hair, 0, 1.095f, 0);
    for (int i = 0; i < 2; i++) {
        float dx = i ? .062f : -.062f;
        cube(g, .028f, .026f, .012f, 0x26352d, dx, .985f, .121f);
        cube(g, .025f, .025f, .016f, 0xdc996e, dx, .94f, .121f);
    }
    for (int i = 0; i < 2; i++) {
        float dx = i ? .077f : -.077f;
        cube(g, .09f, .34f, .1f, 0x34566a, dx, .32f, 0);
        cube(g, .12f, .065f, .18f, 0x2b3e39, dx, .12f, .035f);
    }
    Xf arm = xf_child(g, (V3){.175f * l->handedness, .79f, 0}, m3_euler_xyz(l->arm_x, 0, l->arm_z));
    arm.sy = l->arm_sy;
    cube(&arm, .085f, .17f, .09f, l->shirt, 0, -.075f, 0);
    if (l->pong) {
        Xf elbow = xf_child(&arm, (V3){0, -.15f, 0}, m3_euler_xyz(l->elbow, 0, 0));
        cube(&elbow, .075f, .15f, .075f, l->skin, 0, -.08f, 0);
    } else cube(&arm, .075f, .15f, .075f, l->skin, 0, -.23f, 0);
    cube(g, .085f, .29f, .09f, l->shirt, -.175f * l->handedness, .65f, 0);
    cube(g, .075f, .06f, .075f, l->skin, -.175f * l->handedness, .48f, 0);
    if (l->cup) keg_cup(&arm, -.29f, .035f, .018f);
}

/* Static props (recorded into the yard display list): keg stand + shell, pong table, cacti. */
void party_draw_static(void) {
    M3 I = m3_identity();
    Xf keg = xf_root((V3){(float)PARTY_KEG_X, 0, (float)PARTY_KEG_Z}, I);
    cube(&keg, .68f, .13f, .62f, 0x7a5338, 0, .13f, 0);
    cube(&keg, .09f, .24f, .48f, 0x624730, -.23f, .12f, 0); cube(&keg, .09f, .24f, .48f, 0x624730, .23f, .12f, 0);
    cylinder(&keg, .29f, 0, .65f, 0xaebcb5, .585f, 0);
    static const float bands[4] = {.30f, .42f, .76f, .88f};
    for (int i = 0; i < 4; i++) cylinder(&keg, .31f, 0, .07f, 0x647b78, bands[i], 0);
    cylinder(&keg, .27f, 0, .05f, 0xd8decb, .93f, 0);         /* coupler rim */
    cube(&keg, .16f, .065f, .15f, 0x647b78, 0, .965f, 0);
    cylinder(&keg, .043f, 0, .23f, 0xd8decb, 1.11f, 0);        /* pump */
    cube(&keg, .13f, .04f, .06f, 0x647b78, .105f, .98f, .015f);

    Xf table = xf_root((V3){(float)PARTY_TABLE_X, 0, (float)PARTY_TABLE_Z}, m3_euler_xyz(0, (float)PARTY_TABLE_ROT_Y, 0));
    const float top = (float)PARTY_TABLE_TOP;
    cube(&table, 2.12f, .09f, .88f, 0x705039, 0, top - .07f, 0);
    cube(&table, 2.05f, .025f, .81f, 0x336b53, 0, top - .0125f, 0);
    for (int e = -1; e <= 1; e += 2) {
        cube(&table, .023f, .004f, .76f, 0xe4d8aa, e * .97f, top + .002f, 0);
        for (int s = -1; s <= 1; s += 2) cube(&table, .09f, top - .07f, .09f, 0x5b4032, e * .88f, (top - .07f) / 2, s * .33f);
    }
    for (int s = -1; s <= 1; s += 2) cube(&table, 1.95f, .004f, .022f, 0xe4d8aa, 0, top + .002f, s * .38f);

    static const float cacti[6][3] = {{-5.4f, -6.1f, .7f}, {-4.8f, -9.5f, .5f}, {5.7f, -5.8f, .6f}, {5.9f, -9.5f, .8f}, {-5.7f, -3, .4f}, {5.7f, -2.5f, .5f}};
    for (int i = 0; i < 6; i++) {
        Xf g = xf_root((V3){cacti[i][0], 0, cacti[i][1]}, I);
        float h = cacti[i][2];
        cube(&g, .14f, h, .14f, 0x477c43, 0, h / 2, 0);
        cube(&g, .06f, h * .75f, .15f, 0x65954a, -.045f, h * .4f, 0);
        for (int s = -1; s <= 1; s += 2) {
            cube(&g, .2f, .1f, .11f, 0x477c43, s * .13f, h * .48f, 0);
            cube(&g, .1f, h * .3f, .11f, 0x477c43, s * .2f, h * .6f, 0);
        }
    }
}

static void hose_segment(const Xf *keg, Vec3 a, Vec3 b) {
    V3 d = {(float)(b.x - a.x), (float)(b.y - a.y), (float)(b.z - a.z)};
    float len = sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);
    if (len < 1e-5f) return;
    V3 y = {d.x / len, d.y / len, d.z / len};
    V3 ref = fabsf(y.y) < .9f ? (V3){0, 1, 0} : (V3){1, 0, 0};
    V3 x = {ref.y * y.z - ref.z * y.y, ref.z * y.x - ref.x * y.z, ref.x * y.y - ref.y * y.x};
    float xl = sqrtf(x.x * x.x + x.y * x.y + x.z * x.z); x.x /= xl; x.y /= xl; x.z /= xl;
    V3 z = {x.y * y.z - x.z * y.y, x.z * y.x - x.x * y.z, x.x * y.y - x.y * y.x};
    M3 r = {{{x.x, y.x, z.x}, {x.y, y.y, z.y}, {x.z, y.z, z.z}}};
    Xf s = xf_child(keg, (V3){(float)(a.x + b.x) / 2, (float)(a.y + b.y) / 2, (float)(a.z + b.z) / 2}, r);
    cube(&s, .028f, len + .003f, .028f, 0x374d48, 0, 0, 0);
}

void party_draw(const Party *p, V3 dog_mouth) {
    tex_bind(NULL);
    M3 I = m3_identity();
    /* keg moving parts */
    Xf keg = xf_root((V3){(float)PARTY_KEG_X, 0, (float)PARTY_KEG_Z}, I);
    Xf plunger = xf_child(&keg, (V3){0, (float)p->plunger_y, 0}, I);
    cube(&plunger, .024f, .15f, .024f, 0xb7c8c2, 0, -.045f, 0);
    cube(&plunger, .18f, .045f, .075f, 0x263c39, 0, .035f, 0);
    Xf tap = xf_child(&keg, v3f(p->tap_pos), I);
    cube(&tap, .06f, .055f, .14f, 0x263c39, 0, 0, 0);
    cube(&tap, .035f, .07f, .035f, 0x263c39, 0, -.04f, .08f);
    Xf squeeze = xf_child(&tap, (V3){0, .06f, -.015f}, m3_euler_xyz((float)p->squeeze_rot_x, 0, 0));
    cube(&squeeze, .025f, .08f, .065f, 0x445750, 0, 0, 0);
    for (int i = 0; i < PARTY_HOSE_SEGMENTS; i++) hose_segment(&keg, p->hose[i], p->hose[i + 1]);
    if (p->stream_visible) cube(&keg, .018f, .13f, .018f, 0xe9be59, 0, .655f, .58f);
    if (p->serving_visible) keg_cup(&keg, .53f, .58f, .015f);

    /* keg guest and friend */
    Xf guest = xf_root(v3f(p->guest_pos), m3_euler_xyz((float)p->guest_rot_x, (float)p->guest_rot_y, 0));
    PersonLook gl = {0xd7a349, 0xbd835c, 0x49392c, 1, (float)p->guest_arm_x, (float)p->guest_arm_z, 1, 0, p->guest_cup_visible, false};
    person(&guest, &gl);
    Xf friend = xf_root(v3f(p->friend_pos), m3_euler_xyz(0, (float)p->friend_rot_y, 0));
    PersonLook fl = {0x72a0a0, 0xe3b085, 0x523c30, 1, (float)p->friend_arm_x, (float)p->friend_arm_z, (float)p->friend_arm_scale_y, 0, true, false};
    person(&friend, &fl);

    /* beer pong: players, remaining cups, ball */
    Xf table = xf_root((V3){(float)PARTY_TABLE_X, 0, (float)PARTY_TABLE_Z}, m3_euler_xyz(0, (float)PARTY_TABLE_ROT_Y, 0));
    for (int s = 0; s < 2; s++) {
        const PongPlayerPose *pose = &p->players[s];
        V3 at = s == 0 ? (V3){-1.29f, (float)pose->y, -.22f} : (V3){1.29f, (float)pose->y, .12f};
        Xf pg = xf_child(&table, at, m3_euler_xyz(0, s == 0 ? (float)M_PI / 2 : -(float)M_PI / 2, 0));
        PersonLook pl = s == 0 ? (PersonLook){0xa95549, 0xe8be96, 0x72503c, -1, 0, 0, 1, 0, false, true}
                               : (PersonLook){0x52749a, 0x95674e, 0x303c35, 1, 0, 0, 1, 0, false, true};
        pl.arm_x = (float)pose->shoulder; pl.arm_z = (float)pose->sweep; pl.elbow = (float)pose->elbow;
        person(&pg, &pl);
    }
    for (int side = 0; side < 2; side++)
        for (int i = 0; i < PONG_CUPS; i++) {
            if (!p->cup_visible[side][i]) continue;
            Xf cup = xf_child(&table, v3f(party_rack_cup(side, i)), I);
            cylinder(&cup, .058f, 0, .13f, side ? 0x387dce : 0xdd493c, .065f, 0);
            cylinder(&cup, .066f, .051f, .016f, 0xfff2ce, .13f, 0);
        }
    if (p->ball_visible) gfx_sphere(v3f(p->ball_world), (float)PARTY_BALL_RADIUS, rgb(0xfff8d9));

    /* grill cook */
    Xf cook = xf_root((V3){(float)PARTY_COOK_X, 0, (float)PARTY_COOK_Z}, m3_euler_xyz(0, (float)PARTY_COOK_ROT_Y, 0));
    PersonLook cl = {0xeee0ba, 0xc18b64, 0x3d3830, 1, (float)p->cook_arm_x, (float)p->cook_arm_z, 1, 0, false, false};
    person(&cook, &cl);
    cube(&cook, .26f, .1f, .25f, 0xfff1d1, 0, 1.16f, 0);
    cube(&cook, .16f, .15f, .02f, 0xb75b43, 0, .6f, .1f);
    Xf arm = xf_child(&cook, (V3){.175f, .79f, 0}, m3_euler_xyz((float)p->cook_arm_x, 0, (float)p->cook_arm_z));
    Xf spatula = xf_child(&arm, (V3){0, -.3f, .03f}, I);
    cube(&spatula, .025f, .22f, .025f, 0x6d4b32, 0, -.08f, 0);
    cube(&spatula, .10f, .1f, .025f, 0xb7c8c2, 0, -.24f, 0);
    for (int i = 0; i < PARTY_FOODS; i++) {
        bool steak; Vec3 base = party_food_base(i, &steak);
        bool active = p->flipping && i == p->food_index;
        if (active) base.y += p->food_lift;
        Xf f = xf_root(v3f(base), m3_euler_xyz(0, 0, active ? (float)p->food_rot_z : 0));
        cube(&f, steak ? .14f : .1f, .035f, steak ? .09f : .1f, steak ? 0x9e4938 : 0x75402c, 0, 0, 0);
        for (int k = -1; k <= 1; k++) cube(&f, .012f, .004f, .08f, 0x3c3229, k * .035f, .02f, 0);
    }
    if (p->toss) {
        Vec3 mouth = {dog_mouth.x, dog_mouth.y, dog_mouth.z};
        Xf s = xf_root(v3f(party_snack_position(p, mouth)), m3_euler_xyz(0, 0, (float)p->snack_rot_z));
        cube(&s, .12f, .045f, .11f, 0xd89e4d, 0, 0, 0);
        cube(&s, .115f, .025f, .105f, 0x74422d, 0, -.025f, 0);
    }
}
