#pragma once

#include "common.h"
#include "array.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <harfbuzz/hb-ft.h>

typedef uint32_t utf32_t;

#include "text/text_common.h"

struct Font {
	FT_Face            ft_face;
	hb_font_t         *hb_font;
	enum FontStyle     style;
};

struct TextEngine {
	bool               valid;
	bool               mute_logging;
	struct TextStyle   style; /* Default style */
	int32_t            font_size;

	Array(struct Font) fonts;
	size_t             font_count;

	struct Atlas {
		uint8_t  *bitmap;
		uint32_t  bitmap_size;

		uint32_t w;
		uint32_t h;

		uint32_t dirty;

		uint32_t bin_count;
		struct AtlasBin {
			/* Offset in atlas */
			uint32_t atlas_y;
			uint32_t atlas_h;

			/* Slot size */
			uint32_t w;
			uint32_t h;

			uint32_t slot_count;

			struct GlyphSlot {
				uint32_t reference_count;
				uint32_t valid;
				uint32_t code;
				uint32_t font;
				uint32_t offset;

				/* Size of the bitmap */
				uint32_t w;
				uint32_t h;

				/* Offsets */
				uint32_t x;
				uint32_t y;

				uint32_t atlas_x;
				uint32_t atlas_y; 
			} *slot;
		} *bin;
	} atlas;
};


/***********************
 * TextContext Methods *
 ***********************/

TextEngine text_engine_create(void);

struct GlyphSlot *
text_engine_find_glyph (TextEngine, uint32_t font, uint32_t code);

struct GlyphSlot *
text_engine_cache_glyph(TextEngine, uint32_t font, uint32_t code);

struct TextEngine *
text_engine_get_struct(TextEngine);

void text_engine_export_atlas(TextEngine);

