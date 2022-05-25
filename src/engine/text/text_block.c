#include "text/text_block.h"

#include "common.h"
#include "ppm.h"

#include <errno.h>
#include <iconv.h>
#include <fribidi.h>

static struct TextBlock text_blocks[1024];

struct TextBlock *text_block_get_struct(TextEngine handle)
{
	struct TextBlock *this = &text_blocks[handle-1];
	if (!this->valid) 
		engine_crash("Invalid TextEngine handle");
	return this;
}

TextBlock text_block_alloc(void)
{
	for (ufast32_t i = 0; i < LENGTH(text_blocks); i++) {
		if (!text_blocks[i].valid) {
			text_blocks[i].valid = true;
			return i+1;
		}
	}
	engine_crash("Out of slots");
	return 0; 
}

Array(utf32_t) text_bidi( Array(utf32_t) input ) 
{
	FriBidiParType base = FRIBIDI_PAR_LTR;

	Array(utf32_t) output;
	array_create(output);
	array_reserve(output, array_length(input));

	fribidi_log2vis(
		input, 
		array_length(input)-1,
		&base,
		output,
		NULL,
		NULL,
		NULL
	);

	array_back(output)[0] = 0;

	return output;
}

Array(utf32_t) utf8_to_utf32(const char *utf8)
{
	size_t inlen = strlen(utf8);
	size_t outlen = 0;

	const char *c = utf8;
	while (*c++) 
 	   if ( (*c&0xC0) != 0x80) 
		   outlen++;

	Array(uint32_t) utf32;
	array_create(utf32);
	array_reserve(utf32, outlen+1);

	iconv_t cd = iconv_open("UCS-4LE", "UTF-8"); 

	if (cd == (iconv_t)-1) {
		if (errno == EINVAL) 
			engine_crash("Conversion unavailable");
		else
			engine_crash("iconv_open failed");
	}
	
	char*ci = (char*)utf8;
	char*co = (char*)utf32;
	size_t cil = inlen;
	size_t col = outlen*4;
	
	size_t ret = iconv( cd, 
		&ci, &cil,
		&co, &col
	);
	
	if(ret==(size_t)-1)
		engine_crash("iconv failed");

	return utf32;
}

Array(char) utf32_to_utf8( Array(utf32_t) utf32 )
{
	size_t inlen = array_sizeof(utf32);
	size_t outlen = inlen;

	Array(char) utf8;
	array_create(utf8);
	array_reserve(utf8, outlen+1);

	iconv_t cd = iconv_open("UTF-8", "UCS-4LE"); 

	if (cd == (iconv_t)-1) {
		if (errno == EINVAL) 
			engine_crash("Conversion unavailable");
		else
			engine_crash("iconv_open failed");
	}
	
	char*ci = (void*)utf32;
	char*co = (void*)utf8;
	size_t cil = inlen;
	size_t col = outlen;
	
	size_t ret = iconv( cd, 
		&ci, &cil,
		&co, &col
	);
	
	if (ret==(size_t)-1)
		engine_crash("iconv failed");

	return utf8;
}


void push_text(
	struct TextBlock *restrict this,
	const struct TextStyle   *style,
	uint32_t *utf32,
	uint32_t start,
	uint32_t length)
{
	struct TextEngine *restrict engine = text_engine_get_struct(this->engine);

	if (style == NULL) style = &engine->style;

	struct {
		hb_buffer_t         *buf;
		hb_glyph_info_t     *glyph_info;
		hb_glyph_position_t *glyph_pos;
		uint32_t             glyph_count;
		uint32_t             seek;
		struct Font         *font;
		uint32_t             ctx_index;
	} fc [engine->font_count];
	ufast32_t fnum = 0;
	const uint32_t FONT_MISSING = UINT_MAX;

	/* Try different fonts for each character (aka cluster) until all filled. 
	 * Not all clusters may actually get their own glyph: Harfbuzz may combine 
	 * several characters into a single glyph. */

	uint32_t cluster_font[length];
	uint32_t clusters_missing = 0;

	for (ufast32_t i = 0; i < LENGTH(cluster_font); i++) 
		cluster_font[i] = FONT_MISSING;

	for (ufast32_t i = 0; i < engine->font_count; i++) {
		if (engine->fonts[i].style != 0
		 && engine->fonts[i].style != style->style) 
			continue;
		ufast32_t f = fnum++;
		fc[f].font = &engine->fonts[i];
		fc[f].ctx_index = i;
		fc[f].buf = hb_buffer_create();
		hb_buffer_set_direction (fc[f].buf, HB_DIRECTION_LTR);
		hb_buffer_set_script    (fc[f].buf, HB_SCRIPT_COMMON);
		hb_buffer_add_codepoints(fc[f].buf, utf32, -1, start, length);
		hb_shape(fc[f].font->hb_font,  fc[f].buf, NULL, 0);

		fc[f].glyph_info = hb_buffer_get_glyph_infos(
			fc[f].buf, &fc[f].glyph_count
		);
		fc[f].glyph_pos  = hb_buffer_get_glyph_positions(
			fc[f].buf, &fc[f].glyph_count
		);

		fc[f].seek = 0;

		for (ufast32_t c = 0; c < LENGTH(cluster_font); c++) 
			if (cluster_font[c] == FONT_MISSING)
				cluster_font[c] = f;

		for (ufast32_t g = 0; g < fc[f].glyph_count; g++) {
			uint32_t c = fc[f].glyph_info[g].cluster;

			if (fc[f].glyph_info[g].codepoint == 0 && cluster_font[c] == f) {
				clusters_missing++;
				cluster_font[c] = FONT_MISSING;
			}
		}

		if (clusters_missing == 0)
			break;
	}

	/* Create quads */

	hb_position_t cursor_x = 0;
	hb_position_t cursor_y = 0;

	for (uint32_t cluster = start; cluster < start+length; ) {
		uint32_t f = cluster_font[cluster];

		/* Use unicode replacement character of the primary font */
		if (f == FONT_MISSING) {
			if (utf32[cluster] != '\n')
				log_warn("Missing font, %i", utf32[cluster]);
			f = 0;
		}

		/* Find glyph generated for cluster (if any), store in g */
		uint32_t g = fc[f].seek; 
		while (fc[f].glyph_info[g].cluster != cluster) {
			if (g >= fc[f].glyph_count)
				goto next_cluster;
			g++;
		}
		fc[f].seek = g;

		/* Push glyphs until cluster changes */
		bool space   = (utf32[cluster] == ' ');
		bool newline = (utf32[cluster] == '\n');
		if (newline) this->lines++;

		for (; 
			fc[f].glyph_info[g].cluster == cluster && g < fc[f].glyph_count; 
			g++
		){
			hb_codepoint_t glyphid  = fc[f].glyph_info[g].codepoint;
			
			uint32_t i = fc[f].ctx_index;
			struct GlyphSlot *slot = text_engine_find_glyph(this->engine, i, glyphid);
			if (!slot)
				slot = text_engine_cache_glyph(this->engine, i, glyphid);

			hb_position_t  x_offset  = fc[f].glyph_pos[g].x_offset;
			hb_position_t  y_offset  = fc[f].glyph_pos[g].y_offset;
			hb_position_t  x_advance = fc[f].glyph_pos[g].x_advance;
			hb_position_t  y_advance = fc[f].glyph_pos[g].y_advance;
			cursor_x += x_advance;
			cursor_y += y_advance;

			if (slot == NULL) continue;

			slot->reference_count++;
			array_push(this->quads, (struct GlyphQuad){
				.slot = slot,
				.offset_x = (x_offset)/64+slot->x,
				.offset_y = (y_offset)/64-slot->y,
				.advance_x = x_advance/64,
				.advance_y = y_advance/64,
				.space     = space | newline,
				.newline   = newline,
				.color = {
					style->color[0],
					style->color[1],
					style->color[2],
					style->color[3],
				}
			});
		}

next_cluster:
		if (cluster == start+length-1) break;
		cluster++;
	}
}

void txtblk_unreference_slots(struct TextBlock *block)
{
	for (uint32_t i = 0; i < array_length(block->quads); i++) {
		block->quads[i].slot->reference_count--;
	}
}


void*txtblk_destroy(struct TextBlock *block)
{
	txtblk_unreference_slots(block);

	array_delete(block->quads);
	free(block);
	return NULL;
}


void text_block_align(
	TextBlock      handle,
	enum TextAlign _align, 
	uint32_t _width
){
	struct TextBlock *restrict this = text_block_get_struct(handle);

	this->align     = _align;
	this->max_width = _width;

	text_block_unalign(handle);
	this->aligned = true;
	this->lines = 1;

	if (this->max_width != 0) {
		struct GlyphQuad *last = NULL;
		uint32_t cursor_x = 0;
		uint32_t cursor_word = 0;

		for (uint32_t g = 0; g < array_length(this->quads); g++) {
			struct GlyphQuad *quad = this->quads+g;

			if (quad->newline) {
				cursor_x = 0;
				cursor_word = 0;
				this->lines++;
				continue;
			}

			cursor_word += quad->advance_x;

			if (cursor_x+cursor_word >= this->max_width) {
				
				if (last == NULL) {
					/* Word is longer than max_width */
					last = quad;
				}

				last->newline = true;
				last->alignment_newline = true;
				this->lines++;
				cursor_x = cursor_word;
				cursor_word = this->wrap_indent;
				last = NULL;
				continue;
			}

			if (quad->space) {
				cursor_x += cursor_word;
				cursor_word = 0;
				last = quad;
			}
		}
	}

	if (this->align == TEXT_ALIGN_LEFT
	 && this->wrap_indent) 
	{
		uint32_t add = 0;
		for (size_t g = 0; g < array_length(this->quads); g++) {
			struct GlyphQuad *restrict quad = this->quads+g;
			if (!add && quad->alignment_newline)
				add = this->wrap_indent;
			quad->alignment_x -= add;
		}
	}

	if (this->align == TEXT_ALIGN_RIGHT 
	 || this->align == TEXT_ALIGN_CENTER)
	{
		int32_t  line_width = 0;
		uint32_t line_start = 0;

		for (ufast32_t g = 0; g < array_length(this->quads); g++) {
			struct GlyphQuad *quad = this->quads+g;

			if (quad->newline || g+1 == array_length(this->quads)) {
				int32_t offset = this->max_width - line_width - 2;

				if (this->align == TEXT_ALIGN_CENTER) 
					offset = offset * 0.5;

				for (ufast32_t j = line_start; j < g+1; j++) {
					this->quads[j].offset_x   += offset;
					this->quads[j].alignment_x = offset;
				}

				line_start = g;
				line_width = 0;
				continue;
			}
			line_width += quad->advance_x;
		}
	}
}


void text_block_unalign(TextBlock handle)
{
	struct TextBlock *restrict this = text_block_get_struct(handle);

	if (!this->aligned) 
		return;

	this->aligned = false;
	for (ufast32_t g = 0; g < array_length(this->quads); g++) {
		struct GlyphQuad *quad = this->quads+g;
		
		if (quad->alignment_newline) {
			quad->newline           = false;
			quad->alignment_newline = false;
		}

		quad->offset_x -= quad->alignment_x;
		quad->offset_y -= quad->alignment_y;

		quad->alignment_x = 0;
		quad->alignment_y = 0;
	}
}

void text_block_add(
		TextBlock                        handle,
		const char             *restrict utf8,
		const struct TextStyle *restrict style)
{
	struct TextBlock *restrict this = text_block_get_struct(handle);

	Array(utf32_t) temp = utf8_to_utf32(utf8);
	Array(utf32_t) utf32 = text_bidi(temp);

	push_text(
		this,
		style,
		utf32, 
		0, array_length(utf32)-1
	);

	array_destroy(utf32);
	array_destroy(temp);
}

void text_block_set(
	TextBlock                        handle,
	const char             *restrict utf8,
	const struct TextStyle *restrict style)
{
	struct TextBlock *restrict this = text_block_get_struct(handle);
	txtblk_unreference_slots(this);
	array_clear(this->quads);
	this->lines = 1;
	text_block_add(handle, utf8, style);
	text_block_align(handle, this->align, this->max_width);
}

TextBlock text_block_create(TextEngine engine_handle)
{
	TextBlock handle = text_block_alloc();
	struct TextBlock  *restrict this   = text_block_get_struct(handle);
	struct TextEngine *restrict engine = text_engine_get_struct(engine_handle);
	
	array_create(this->quads);
	this->engine = engine_handle;
	this->line_height = engine->font_size; 
	
	return handle;
}

uint32_t text_block_get_lines(TextBlock handle)
{
	struct TextBlock *restrict this = text_block_get_struct(handle);
	return this->lines;
}
uint32_t text_block_get_line_height(TextBlock handle)
{
	struct TextBlock *restrict this = text_block_get_struct(handle);
	return this->line_height;
}


void text_block_set_wrap_indent(TextBlock handle, uint32_t val)
{
	struct TextBlock *restrict this = text_block_get_struct(handle);
	this->wrap_indent = val;
}
