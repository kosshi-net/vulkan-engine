#pragma once

#include "common.h"
#include "array.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <harfbuzz/hb-ft.h>

typedef uint32_t utf32_t;

/* Structures */

enum FontFamily {
	FONT_SANS_16,
	FONT_MONO_16,
};


struct TextVertex {
	float   pos  [2];
	float   uv   [2];
	uint8_t color[4];
};

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
};

enum FontStyle {
	FONT_STYLE_NORMAL = 0,
	FONT_STYLE_BOLD   = (1<<0),
	FONT_STYLE_ITALIC = (1<<1),
};

struct TextStyle {
	uint8_t            color[4];
	enum FontStyle     style;
};

struct Font {
	FT_Face            ft_face;
	hb_font_t         *hb_font;
	enum FontStyle     style;
};



struct TextContext {
	struct Atlas atlas;

	bool         mute_logging;

	struct TextStyle style;

	Array(struct Font) fonts;
	size_t font_count;

	int32_t font_size;
	
	int32_t root_x;
	int32_t root_y;

	int32_t cursor_x;
	int32_t cursor_y;

	bool scissor_enable;
	struct {
		int32_t x; 
		int32_t y;
		int32_t w;
		int32_t h;
	} scissor;

	Array(struct TextBlok *) block_buffer;

	Array(struct TextVertex) vertex_buffer;
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

	int32_t alignment_x;
	int32_t alignment_y;
	bool    alignment_newline;

	uint8_t color[4];
};


struct TextBlock {
	struct TextContext *ctx;

	enum TextAlign {
		TEXT_ALIGN_LEFT,
		TEXT_ALIGN_RIGHT,
		TEXT_ALIGN_CENTER,
		TEXT_ALIGN_JUSTIFY,
	} align;

	bool aligned;

	uint32_t max_width;
	uint32_t lines;

	Array(struct GlyphQuad) quads;
};



/***********************
 * TextContext Methods *
 ***********************/

struct TextContext *
txtctx_create(enum FontFamily);

/* 
 * Queue block for rendering 
 */
void txtctx_add(struct TextBlock *block);

/* 
 * Clear queued blocks 
 */
void txtctx_clear(struct TextContext *ctx);

/* 
 * Reset the cursor and move it to a new location 
 */
void txtctx_set_root(struct TextContext *ctx, uint32_t x, uint32_t y);

/* 
 * Insert a new line
 */
void txtctx_newline(struct TextContext *ctx);

/*
 * Set scissor. Used for culling.
 */

void txtctx_set_scissor(
	struct TextContext *ctx,
	int32_t x, int32_t y, int32_t w, int32_t h
);

/*********************
 * TextBlock Methods *
 *********************/

/* 
 * Create and allocate a textblock. 
 */
struct TextBlock *
txtblk_create(struct TextContext *ctx, char*);

/* 
 * Change the text of the block. Style can be NULL.
 */
void txtblk_edit(struct TextBlock*, const char*); // DEPRICATED
void txtblk_set_text(struct TextBlock*, const char*, struct TextStyle*);

/*
 * Add text to the block. Style can be NULL.
 */
void txtblk_add_text(struct TextBlock*, const char*, struct TextStyle*);

/* 
 * Set alignment and modify quad positions to match.
 * Is called by txtblk_edit with previously set values.
 * The defaults TEXT_ALIGN_LEFT and width=0 make this function a no-op. 
 *
 * If width is not 0, line breaks are inserted.
 * If width is 0, alignment is performed relative to the cursor position.
 */
void txtblk_align(struct TextBlock*, enum TextAlign, uint32_t width );

/* 
 * Undo the modifications done by txtblk_align. Does not clear the alignment
 * settings, so txtblk_edit will restore the previous alignment.
 * Meant mostly for internal use (called on repeated calls to txtblk_align)
 */
void  txtblk_unalign( struct TextBlock *);

void *txtblk_destroy(struct TextBlock *);

/* Convenience function */
static inline 
struct TextStyle *
textstyle_set(
	struct TextStyle *style, 
	uint32_t          color_hex,
	enum FontStyle    font_style)
{
	style->color[0] = color_hex >> 24;
	style->color[1] = color_hex >> 16;
	style->color[2] = color_hex >> 8;
	style->color[3] = color_hex >> 0;
	style->style    = font_style;
	return style;
}

/*****************
 * Other Methods *
 *****************/

uint16_t *txt_create_shared_index_buffer(size_t max_glyphs, size_t *size);

