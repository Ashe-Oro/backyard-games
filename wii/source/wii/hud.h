#ifndef BYC_HUD_H
#define BYC_HUD_H

#include "../core/game.h"
#include "gfx.h"
#include "scene.h"

void hud_draw(const Game *g, const Input *in);
void hud_board_view_rect(int *x, int *y, int *w, int *h);
bool hud_board_view_visible(const Game *g);
/* Optional IR cursor in 640x480 space. Never required for input. */
void hud_draw_pointer(float x, float y);

#endif
