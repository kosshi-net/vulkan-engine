#pragma once

#include "engine.h"
#include "engine/text/text.h"
#include "engine/widget/widget_renderer.h"

void term_create(TextEngine);
void term_update(struct Frame *frame);

TextGeometry term_create_gfx(TextRenderer renderer, WidgetRenderer widget);

bool term_mouse(int action);

void term_destroy(void);

