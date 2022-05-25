#pragma once 

#include "text/text_common.h"
#include "text_engine.h"

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

typedef Handle TextBlock;
struct TextBlock {
	bool valid;

	TextEngine engine;

	enum TextAlign {
		TEXT_ALIGN_LEFT,
		TEXT_ALIGN_RIGHT,
		TEXT_ALIGN_CENTER,
		TEXT_ALIGN_JUSTIFY,
	} align;

	bool aligned;

	uint32_t max_width;
	uint32_t lines;
	uint32_t line_height;
	uint32_t wrap_indent;

	Array(struct GlyphQuad) quads;
};


/***********
 * Methods *
 ***********/

TextBlock text_block_create(TextEngine);
TextBlock text_block_destroy(TextBlock);

void      text_block_set(TextBlock, const char*, const struct TextStyle*);
void      text_block_add(TextBlock, const char*, const struct TextStyle*);

struct TextBlock *
text_block_get_struct(TextBlock);

/* 
 * Set alignment and modify quad positions to match.
 * Is called by txtblk_edit with previously set values.
 * The defaults TEXT_ALIGN_LEFT and width=0 make this function a no-op. 
 *
 * If width is not 0, line breaks are inserted.
 * If width is 0, alignment is performed relative to the cursor position.
 */
void text_block_align(TextBlock, enum TextAlign, uint32_t width);

void      text_block_set_line_height(TextBlock, uint32_t);
void      text_block_set_wrap_indent(TextBlock, uint32_t);
uint32_t  text_block_get_lines(TextBlock);
uint32_t  text_block_get_line_height(TextBlock);

/* 
 * Undo the modifications done by txtblk_align. Does not clear the alignment
 * settings, so txtblk_edit will restore the previous alignment.
 * Meant mostly for internal use (called on repeated calls to txtblk_align)
 */
void text_block_unalign(TextBlock);

/* Convenience function */
static inline struct TextStyle *textstyle_set(
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

