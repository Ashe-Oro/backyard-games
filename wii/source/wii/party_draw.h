#ifndef BYC_PARTY_DRAW_H
#define BYC_PARTY_DRAW_H

#include "../core/party.h"
#include "gfx.h"

/* Static party props (keg body, pong table, cacti); call while recording the yard display list. */
void party_draw_static(void);
/* Moving party NPCs. dog_mouth: world point the cook's burger toss lands on. */
void party_draw(const Party *p, V3 dog_mouth);

#endif
