#pragma once

#include "engine.h"
#include "engine/text/text.h"

void term_create(TextEngine);
void term_create_gfx(TextRenderer);
void term_update(struct Frame *frame);
void term_destroy(void);

