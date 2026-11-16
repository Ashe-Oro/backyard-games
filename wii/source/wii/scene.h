#ifndef BYC_SCENE_H
#define BYC_SCENE_H

#include "../core/game.h"
#include "gfx.h"

void scene_init(void);
void scene_fx(const Fx *fx);
void scene_update(const Game *g, double dt);
/* held_dip: 0..1 cosmetic dip/arc while B is held (SPEC §4.8). */
void scene_draw(const Game *g, float held_dip);
void scene_draw_board_view(const Game *g, int x, int y, int w, int h);
bool scene_bag_screen(const Game *g, float *sx, float *sy);
void scene_portrait(int level, Tex **out);

#endif
