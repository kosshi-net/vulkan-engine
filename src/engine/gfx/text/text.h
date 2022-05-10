#pragma once

#include "common.h"
#include "array.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <harfbuzz/hb-ft.h>

typedef uint32_t utf32_t;

/* Structures */

struct Atlas {
	uint8_t  *bitmap;
	uint32_t  bitmap_size;

	uint32_t w;
	uint32_t h;

	uint32_t dirty;

	uint32_t bin_count;
	struct AtlasBin {
		// Offset in atlas
		uint32_t atlas_y;
		uint32_t atlas_h;

		// Slot size
		uint32_t w;
		uint32_t h;

		uint32_t slot_count;

		struct GlyphSlot {
			uint32_t reference_count;
			uint32_t valid;
			uint32_t code;
			uint32_t font;
			uint32_t offset;

			// Size of the bitmap
			uint32_t w;
			uint32_t h;

			// Offsets
			uint32_t x;
			uint32_t y;

			uint32_t atlas_x;
			uint32_t atlas_y; 
		} *slot;
	} *bin;
};

struct TextContext {
	struct Atlas atlas;

	size_t       font_count;

	struct Font {
		FT_Face    ft_face;
		hb_font_t *hb_font;
	} *fonts;

	int32_t font_size;

	//Array(struct TextBlock *) blocks;
	
	int32_t root_x;
	int32_t root_y;

	int32_t cursor_x;
	int32_t cursor_y;

	Array(struct TextBlok *) block_buffer;

	Array(float)    vertex_buffer;
	size_t glyph_count;

};

struct GlyphQuad {
	struct GlyphSlot *slot;
	int32_t offset_x;
	int32_t offset_y;

	int32_t advance_x;
	int32_t advance_y;

	bool    space; 
	bool    newline; 
};

struct TextBlock {
	struct TextContext *ctx;

	enum TextAlign {
		TEXT_ALIGN_LEFT,
		TEXT_ALIGN_RIGHT,
		TEXT_ALIGN_CENTER,
		TEXT_ALIGN_JUSTIFY,
	} align;

	uint32_t max_width;

	Array(uint32_t)         utf32;
	Array(struct GlyphQuad) quads;
};

/* TextContext Methods */

struct TextContext * 
txtctx_create(void);

void txtctx_add     (struct TextBlock *block);
void txtctx_clear   (struct TextContext *restrict ctx);
void txtctx_set_root(struct TextContext *restrict ctx, uint32_t x, uint32_t y);
void txtctx_newline (struct TextContext *restrict ctx);


/* TextBlock Methods */

struct TextBlock *
txtblk_create(struct TextContext *ctx, char*);

void 
txtblk_edit(struct TextBlock*, char*);

void 
txtblk_destroy(struct TextBlock **);

/* Other Methods */

uint16_t *
txt_create_shared_index_buffer(size_t max_glyphs, size_t *size);

void text_test(void);
