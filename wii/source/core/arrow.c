#include "arrow.h"
#include <math.h>

double view_ui_width(double aspect) { return aspect > 0 ? VIEW_H * aspect : VIEW_W; }

bool view_project(Vec3 w, double aspect, double *sx, double *sy) {
    /* lookAt with up = +Y, then a symmetric perspective (same matrices as guLookAt/guPerspective). */
    double fx = VIEW_AT_X - VIEW_EYE_X, fy = VIEW_AT_Y - VIEW_EYE_Y, fz = VIEW_AT_Z - VIEW_EYE_Z;
    double fl = sqrt(fx * fx + fy * fy + fz * fz);
    fx /= fl; fy /= fl; fz /= fl;
    double rx = -fz, rz = fx, rl = sqrt(rx * rx + rz * rz);     /* right = forward x up */
    rx /= rl; rz /= rl;
    double ux = -rz * fy, uy = rz * fx - rx * fz, uz = rx * fy; /* up = right x forward */
    double px = w.x - VIEW_EYE_X, py = w.y - VIEW_EYE_Y, pz = w.z - VIEW_EYE_Z;
    double ex = px * rx + pz * rz, ey = px * ux + py * uy + pz * uz, depth = px * fx + py * fy + pz * fz;
    if (depth <= 0.001) return false;
    double f = 1 / tan(VIEW_FOV_DEG * BYC_PI / 360);
    *sx = (f / aspect * ex / depth + 1) * view_ui_width(aspect) / 2;
    *sy = (1 - f * ey / depth) * VIEW_H / 2;
    return true;
}

Vec3 view_held_bag_center(double release_x, double dip) {
    return (Vec3){release_x, BYC_REL_Y - dip * .05, BYC_REL_Z + dip * .04};
}

bool view_bag_bounds(Vec3 eye, float scale, float near, float proj_x, float proj_y,
                     ViewBounds *bounds) {
    /* Every orientation fits in this sphere, including the puffed faces. Project an
     * enclosing box to get conservative bounds without transforming the mesh twice. */
    const float half = (float)BYC_BAG * .5f, height = (float)BYC_BAG_H;
    float radius = sqrtf(2 * half * half + height * height) * scale;
    float closest = -(float)eye.z - radius, farthest = -(float)eye.z + radius;
    if (!isfinite(eye.x) || !isfinite(eye.y) || !isfinite(closest) || closest <= near)
        return false;

    float low_x = (float)eye.x - radius, high_x = (float)eye.x + radius;
    float low_y = (float)eye.y - radius, high_y = (float)eye.y + radius;
    bounds->left = fmaxf(-1, proj_x * low_x / (low_x < 0 ? closest : farthest));
    bounds->bottom = fmaxf(-1, proj_y * low_y / (low_y < 0 ? closest : farthest));
    bounds->right = fminf(1, proj_x * high_x / (high_x > 0 ? closest : farthest));
    bounds->top = fminf(1, proj_y * high_y / (high_y > 0 ? closest : farthest));
    return bounds->left < bounds->right && bounds->bottom < bounds->top;
}

void view_board_inset(double aspect, int *x, int *y, int *w, int *h) {
    if (aspect > 1.5) { *x = 12; *y = 28; *w = 136; *h = 100; }
    else { *x = 28; *y = 92; *w = 132; *h = 150; }
}

double arrow_length(double power) {
    double p = fmax(0, fmin(1, power));
    return fmax(ARROW_MIN_PX, p * ARROW_MAX_PX);
}

double arrow_alpha(double age_ms, bool live) {
    if (live || age_ms <= ARROW_HOLD_MS) return 1;
    return fmax(0, 1 - (age_ms - ARROW_HOLD_MS) / ARROW_FADE_MS);
}

bool arrow_shape(double release_x, double yaw, double power, double aspect, ArrowShape *out) {
    /* 1.0.7 showDirectionArrow: project a unit launch bearing from the release point, normalise it
     * in screen space and scale to the power length. Never hole-facing, never amplified. */
    Vec3 from = {release_x, BYC_REL_Y, BYC_REL_Z};
    Vec3 to = {release_x + sin(yaw), BYC_REL_Y, BYC_REL_Z - cos(yaw)};
    double tx, ty;
    if (!view_project(from, aspect, &out->ox, &out->oy) || !view_project(to, aspect, &tx, &ty)) return false;
    double dx = tx - out->ox, dy = ty - out->oy, d = hypot(dx, dy);
    if (d < 1e-9) return false;
    out->dx = dx / d; out->dy = dy / d;
    out->length = arrow_length(power);
    out->tx = out->ox + out->dx * out->length; out->ty = out->oy + out->dy * out->length;
    out->ex = out->ox + out->dx * ARROW_MAX_PX; out->ey = out->oy + out->dy * ARROW_MAX_PX;
    return true;
}
