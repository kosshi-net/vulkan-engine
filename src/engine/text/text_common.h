#pragma once 

#include "common.h"

enum FontStyle {
	FONT_STYLE_NORMAL    = 0,
	FONT_STYLE_BOLD      = (1<<0),
	FONT_STYLE_ITALIC    = (1<<1),
	FONT_STYLE_MONOSPACE = (1<<2),
};

struct TextStyle {
	uint8_t            color[4];
	enum FontStyle     style;
};

typedef Handle TextEngine;
typedef Handle TextGeometry;
typedef Handle TextRenderer;
typedef Handle TextBlock;
