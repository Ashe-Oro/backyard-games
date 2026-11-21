#ifndef BYC_HUD_H
#define BYC_HUD_H

#include "../core/game.h"
#include "gfx.h"
#include "scene.h"

void hud_draw(const Game *g, const Input *in);
void hud_board_view_rect(int *x, int *y, int *w, int *h);
bool hud_board_view_visible(const Game *g);
void hud_draw_wiimote_debug(const Input *in, const InputSample *latest);

#endif
