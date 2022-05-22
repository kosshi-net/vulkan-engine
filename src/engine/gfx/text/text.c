#include "gfx/text/text.h"

#include "engine.h"
#include "common.h"
#include "log/log.h"
#include "ppm.h"
#include <harfbuzz/hb-ft.h>
#include <GLFW/glfw3.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "array.h"

#include <errno.h>
#include <iconv.h>

#include <fribidi.h>

static FT_Library ft;

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


void text_create_bin( 
	struct TextContext *ctx, 
	uint32_t bin_num, 
	uint32_t slot_rows,
	uint32_t slot_w,
	uint32_t slot_h
);

struct TextContext *txtctx_create(enum FontFamily family)
{
	Array(char*) fonts;
	array_create(fonts);

	switch (family) {
	case FONT_SANS_16:
		array_push(fonts, "fonts/NotoSans-Regular.ttf");
		array_push(fonts, "fonts/NotoSansCJK-Medium.ttc");
		array_push(fonts, "fonts/NotoSansArabic-Regular.ttf");
		array_push(fonts, "fonts/NotoSansHebrew-Regular.ttf");
		break;
	case FONT_MONO_16:
		array_push(fonts, "fonts/TerminusTTF.ttf");
		array_push(fonts, "fonts/sazanami-gothic.ttf");
		break;
	default:
		engine_crash("Undefined font family");
	}

	uint32_t ret; 
	struct TextContext *restrict ctx = calloc(sizeof(struct TextContext), 1);

	ctx->font_count = array_length(fonts);
	ctx->font_size = 16;
	ctx->fonts = calloc(ctx->font_count, sizeof(struct Font));

	memset(ctx->style.color, 0xFF, LENGTH(ctx->style.color));

	array_create(ctx->vertex_buffer);
	array_create(ctx->block_buffer);

	if (!ft) {
		ret = FT_Init_FreeType(&ft);
		if(ret) 
			engine_crash("FT_Init_FreeType failed");
	}

	for (ufast32_t i = 0; i < ctx->font_count; i++) {
		struct Font *f = &ctx->fonts[i];
		ret = FT_New_Face(ft,
			fonts[i],
			0,
			&f->ft_face
		);

		log_info("%s sizes:", fonts[i]);
		for (fast32_t j = 0; j < f->ft_face->num_fixed_sizes; j++)
		{
			log_info("%ix%i", 
				f->ft_face->available_sizes[j].width,
				f->ft_face->available_sizes[j].height
			);
		}

		if(ret) engine_crash("FT_New_face failed");

		ret = FT_Set_Pixel_Sizes(f->ft_face, 0, ctx->font_size);

		if(ret) log_error("FT_Set_Pixel_Sizes failed");
		f->hb_font = hb_ft_font_create_referenced(f->ft_face);
		hb_ft_font_set_funcs(f->hb_font);
	}

	/* ATLAS */
	
	struct Atlas *atlas = &ctx->atlas;

	atlas->w = 512;
	atlas->h = 512;

	atlas->bitmap_size = atlas->w * atlas->h;
	atlas->bitmap = calloc( atlas->bitmap_size, sizeof(uint8_t) );

	atlas->bin_count = 3;
	atlas->bin = calloc( sizeof(struct AtlasBin), atlas->bin_count );
	
	fast32_t bin = 0;
	fast32_t page = 16 * 2;
	fast32_t pages = atlas->h / page;
	fast32_t pages_small = 1;
	fast32_t pages_big   = 4;

	text_create_bin(
		ctx, bin++,
		4 * pages_small,
		page/4, page/4
	);

	text_create_bin(
		ctx, bin++,
		2 * (pages - pages_small - pages_big),
		page/2, page/2
	);

	text_create_bin(
		ctx, bin++,
		1 * pages_big,
		page, page
	);

	return ctx;
}

void text_create_bin( 
	struct TextContext *ctx, 
	uint32_t bin_num, 
	uint32_t slot_rows,
	uint32_t slot_w,
	uint32_t slot_h
){
	struct Atlas    *atlas = &ctx->atlas;
	struct AtlasBin *bin   = &ctx->atlas.bin[bin_num];

	int atlas_y = 0;
	if (bin_num) 
		atlas_y = bin[-1].atlas_y + bin[-1].atlas_h;

	bin->atlas_y = atlas_y;
	bin->w = slot_w;
	bin->h = slot_h;
	bin->atlas_h = slot_rows * slot_h;

	uint32_t slot_cols = (ctx->atlas.w / bin->w);
	bin->slot_count = slot_rows * slot_cols;
	bin->slot = calloc(sizeof(struct GlyphSlot), bin->slot_count);

	for (uint32_t slot = 0; slot < bin->slot_count; slot++){
		
		uint32_t slot_x = (slot % slot_cols);
		uint32_t slot_y = (slot / slot_cols);

		bin->slot[slot].atlas_x = (slot_x*bin->w);
		bin->slot[slot].atlas_y = (slot_y*bin->h) + bin->atlas_y;

		bin->slot[slot].offset = 
			slot_x*bin->w + 
			(slot_y * atlas->w * bin->h) +
			(bin->atlas_y * atlas->w); 
		
		/* Fill the atlas with a checkerboard pattern */
		if(1)
		if (slot_x%2 == slot_y%2) {
			for( uint32_t x = 0; x < bin->w; x++ )
			for( uint32_t y = 0; y < bin->h; y++ ){
				atlas->bitmap[ bin->slot[slot].offset + x + atlas->w*y ] = 0x11;
			}
		}
	}
}

struct GlyphSlot *text_find_glyph( 
		struct TextContext *ctx, 
		uint32_t font, uint32_t code 
){
	for (uint32_t bin  = 0; bin  < ctx->atlas.bin_count;           bin++)
	for (uint32_t s    = 0; s    < ctx->atlas.bin[bin].slot_count; s++) {
		struct GlyphSlot *slot = &ctx->atlas.bin[bin].slot[s];
		if (slot->code == code 
		 && slot->font == font
		 && slot->valid 
		) return slot;
	}
	return NULL;
}

bool glyph_bit(const FT_GlyphSlot glyph, const int x, const int y)
{
	int32_t pitch = abs(glyph->bitmap.pitch);
	uint8_t *row = &glyph->bitmap.buffer[pitch * y];
    uint8_t c = row[x >> 3];
    return (c & (128 >> (x & 7))) != 0;
}

struct GlyphSlot *text_cache_glyph( 
	struct TextContext *ctx, 
	uint32_t font_id,
	uint32_t code
){
	struct Atlas    *atlas = &ctx->atlas;
	struct AtlasBin *bin;
	struct GlyphSlot *slot;
	struct Font *font = ctx->fonts+font_id;
	
	int ret = FT_Load_Glyph(font->ft_face, code, FT_LOAD_DEFAULT);
	FT_GlyphSlot glyph =    font->ft_face->glyph;
	FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL);
	if(ret) goto error;

	uint32_t h = glyph->bitmap.rows; 
	uint32_t w = glyph->bitmap.width;

	uint32_t bin_num ;
	for ( bin_num = 0; bin_num < atlas->bin_count; bin_num++ ){
		if( h <= atlas->bin[bin_num].h
		&&  w <= atlas->bin[bin_num].w) goto bin_found;
	}
	if (!ctx->mute_logging) 
		log_error("No bin big enough! %i %i",w,h);
	goto error;
bin_found:
	/* Use any free slot first, evict refcount=0 from cache as last resort */
	bin = &ctx->atlas.bin[bin_num];
	uint32_t slot_id = UINT_MAX;
	for (uint32_t i = 0; i < bin->slot_count; i++) {
		if (bin->slot[i].valid == 0) {
			slot_id = i;
			goto slot_found;
		} else if (bin->slot[i].reference_count == 0) {
			slot_id = i;
		}
	}
	if (slot_id != UINT_MAX) goto slot_found;
	if (!ctx->mute_logging) 
		log_error("Out of slots! %i %i", w, h);
	goto error;
slot_found:
	slot = &bin->slot[slot_id];
	slot->code = code;
	slot->font = font_id;

	slot->x = glyph->bitmap_left;
	slot->y = glyph->bitmap_top;
	slot->w = w;
	slot->h = h;

	switch (glyph->bitmap.pixel_mode) {
	case FT_PIXEL_MODE_GRAY: 
		for (ufast32_t x = 0; x < w; x++)
		for (ufast32_t y = 0; y < h; y++) {
			uint8_t pixel = glyph->bitmap.buffer[ w * y + x ];
			atlas->bitmap [ slot->offset + (x) + (y*atlas->w) ] = pixel;
		}
		break;
	case FT_PIXEL_MODE_MONO:
		for (ufast32_t x = 0; x < w; x++)
		for (ufast32_t y = 0; y < h; y++) {
			uint8_t pixel = glyph_bit(glyph, x, y);
			atlas->bitmap [ slot->offset + (x) + (y*atlas->w) ] = pixel*0xFF;
		}
		break;
	default:
		if (!ctx->mute_logging) 
			log_error("Unsupported pixel format %i", glyph->format);
		goto error;
	}

	slot->valid = true;
	atlas->dirty = 1;

	return slot;
error:
	if (!ctx->mute_logging) 
		log_warn("Further logs for this text context will be muted");
	ctx->mute_logging = true;
	return NULL;
}

void push_text(
	struct TextContext *ctx,
	struct TextBlock   *block,
	struct TextStyle   *style,
	uint32_t *utf32,
	uint32_t start,
	uint32_t length
){
	struct {
		hb_buffer_t         *buf;
		hb_glyph_info_t     *glyph_info;
		hb_glyph_position_t *glyph_pos;
		uint32_t             glyph_count;
		uint32_t             seek;
	} fc [ctx->font_count];
	
	const uint32_t FONT_MISSING = UINT_MAX;

	/* Try different fonts for each character (aka cluster) until all filled. 
	 * Not all clusters may actually get their own glyph: Harfbuzz may combine 
	 * several characters into a single glyph. */

	uint32_t cluster_font[length];
	uint32_t clusters_missing = 0;

	for (ufast32_t i = 0; i < LENGTH(cluster_font); i++) 
		cluster_font[i] = FONT_MISSING;

	for (ufast32_t f = 0; f < ctx->font_count; f++) {
		fc[f].buf = hb_buffer_create();
		hb_buffer_set_direction (fc[f].buf, HB_DIRECTION_LTR);
		hb_buffer_set_script    (fc[f].buf, HB_SCRIPT_COMMON);
		hb_buffer_add_codepoints(fc[f].buf, utf32, -1, start, length);
		hb_shape(ctx->fonts[f].hb_font,  fc[f].buf, NULL, 0);

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
		int32_t f = cluster_font[cluster];

		/* Use unicode replacement character of the primary font */
		if (f == -1) f = 0;

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

		for (; 
			fc[f].glyph_info[g].cluster == cluster && g < fc[f].glyph_count; 
			g++
		){
			hb_codepoint_t glyphid  = fc[f].glyph_info[g].codepoint;

			struct GlyphSlot *slot = text_find_glyph(ctx, f, glyphid);
			if (!slot)
				slot = text_cache_glyph(ctx, f, glyphid);

			hb_position_t  x_offset  = fc[f].glyph_pos[g].x_offset;
			hb_position_t  y_offset  = fc[f].glyph_pos[g].y_offset;
			hb_position_t  x_advance = fc[f].glyph_pos[g].x_advance;
			hb_position_t  y_advance = fc[f].glyph_pos[g].y_advance;
			cursor_x += x_advance;
			cursor_y += y_advance;

			if (slot == NULL) continue;

			slot->reference_count++;
			array_push(block->quads, (struct GlyphQuad){
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

struct TextBlock *txtblk_create(struct TextContext *ctx, char *utf8)
{
	struct TextBlock *block = calloc(sizeof(*block), 1);
	
	array_create(block->quads);

	block->ctx = ctx;

	if (utf8) {
		txtblk_edit(block, utf8);
	}

	return block;
}

void txtblk_add_text(
	struct TextBlock *restrict block, 
	const char       *restrict utf8,
	struct TextStyle *restrict style
){
	Array(utf32_t) temp = utf8_to_utf32(utf8);
	Array(utf32_t) utf32 = text_bidi(temp);

	push_text(
		block->ctx,
		block,
		style==NULL ? &block->ctx->style : style,
		utf32, 
		0, array_length(utf32)-1
	);

	array_destroy(utf32);
	array_destroy(temp);
}

void txtblk_set_text(
	struct TextBlock *restrict block, 
	const char       *restrict utf8,
	struct TextStyle *restrict style
){
	txtblk_unreference_slots(block);
	array_clear(block->quads);
	txtblk_add_text(block, utf8, style);
	txtblk_align(block, block->align, block->max_width);
}

void txtblk_edit(struct TextBlock *restrict block, const char *restrict utf8)
{
	txtblk_unreference_slots(block);

	Array(utf32_t) temp = utf8_to_utf32(utf8);
	Array(utf32_t) utf32 = text_bidi(temp);

	array_clear(block->quads);
	push_text(
		block->ctx,
		block,
		&block->ctx->style,
		utf32, 
		0, array_length(utf32)-1
	);

	array_destroy(utf32);
	array_destroy(temp);

	block->aligned = false;
	
	txtblk_align(block, block->align, block->max_width);
}

void*txtblk_destroy(struct TextBlock *block)
{
	txtblk_unreference_slots(block);

	array_delete(block->quads);
	free(block);
	return NULL;
}


void txtblk_align(
	struct TextBlock*block, 
	enum TextAlign _align, 
	uint32_t _width
){
	block->align     = _align;
	block->max_width = _width;

	txtblk_unalign(block);
	block->aligned = true;

	if (block->max_width != 0) {
		struct GlyphQuad *last = NULL;
		uint32_t cursor_x = 0;
		uint32_t cursor_word = 0;

		for (uint32_t g = 0; g < array_length(block->quads); g++) {
			struct GlyphQuad *quad = block->quads+g;

			if (quad->newline) {
				cursor_x = 0;
				cursor_word = 0;
				continue;
			}

			cursor_word += quad->advance_x;

			if (cursor_x+cursor_word >= block->max_width && last) {
				last->newline = true;
				last->alignment_newline = true;
				cursor_x = cursor_word;
				cursor_word = 0;
				continue;
			}

			if (quad->space) {
				cursor_x += cursor_word;
				cursor_word = 0;
				last = quad;
			}
		}
	}

	if (block->align == TEXT_ALIGN_RIGHT 
	 || block->align == TEXT_ALIGN_CENTER
	){
		int32_t  line_width = 0;
		uint32_t line_start = 0;

		for (ufast32_t g = 0; g < array_length(block->quads); g++) {
			struct GlyphQuad *quad = block->quads+g;

			if (quad->newline || g+1 == array_length(block->quads)) {
				int32_t offset = block->max_width - line_width - 2;

				if (block->align == TEXT_ALIGN_CENTER) 
					offset = offset * 0.5;

				for (ufast32_t j = line_start; j < g+1; j++) {
					block->quads[j].offset_x   += offset;
					block->quads[j].alignment_x = offset;
				}

				line_start = g;
				line_width = 0;
				continue;
			}
			line_width += quad->advance_x;
		}
	}
}


void txtblk_unalign( struct TextBlock *restrict block)
{
	if (!block->aligned) 
		return;

	block->aligned = false;
	for (ufast32_t g = 0; g < array_length(block->quads); g++) {
		struct GlyphQuad *quad = block->quads+g;
		
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

void txtblk_set_alignment(
	struct TextBlock *block, 
	enum TextAlign align, 
	uint32_t max_width 
){
	block->align = align;
	block->max_width = max_width;
}

void txtctx_newline(struct TextContext *restrict ctx)
{
	ctx->cursor_x = 0;
	ctx->cursor_y += ctx->font_size;
}

void txtctx_set_root(struct TextContext *restrict ctx, uint32_t x, uint32_t y)
{
	ctx->cursor_x = 0;
	ctx->cursor_y = 0;
	ctx->root_x = x;
	ctx->root_y = y;
}

void txtctx_clear(struct TextContext *restrict ctx)
{
	array_clear( ctx->vertex_buffer );

	ctx->cursor_x = 0;
	ctx->cursor_y = 0;
	ctx->root_x = 0;
	ctx->root_y = 0;

	ctx->glyph_count = 0;
}

void txtctx_add(struct TextBlock *block)
{
	static const uint32_t quad_vertex[4][3] = {
		{0, 0, 0}, 
		{0, 1, 0}, 
		{1, 0, 0}, 
		{1, 1, 0}, 
	};

	struct TextContext *ctx = block->ctx;

	for (ufast32_t g = 0; g < array_length(block->quads); g++)
	{
		struct GlyphQuad *restrict quad = &block->quads[g];
		int32_t x = quad->offset_x + ctx->root_x + ctx->cursor_x;
		int32_t y = quad->offset_y + ctx->root_y + ctx->cursor_y 
			+ ctx->font_size;

		ctx->cursor_x += quad->advance_x;
		ctx->cursor_y += quad->advance_y;

		if (quad->newline) {
			ctx->cursor_x = 0;
			ctx->cursor_y += ctx->font_size;
		}

		if (quad->space) 
			continue;

		ctx->glyph_count++;
		
		int32_t w = quad->slot->w;
		int32_t h = quad->slot->h;

		float uv_x = quad->slot->atlas_x / (float)ctx->atlas.w;
		float uv_y = quad->slot->atlas_y / (float)ctx->atlas.h;
		float uv_w = w / (float)ctx->atlas.w;
		float uv_h = h / (float)ctx->atlas.h;

		for (int i = 0; i < 4; i++) {
			struct TextVertex vert;
			vert.pos[0] = (x + w * quad_vertex[i][0]);
			vert.pos[1] = (y + h * quad_vertex[i][1]);

			vert.uv[0] = (uv_x + uv_w * (float)quad_vertex[i][0]);
			vert.uv[1] = (uv_y + uv_h * (float)quad_vertex[i][1]);

			memcpy(vert.color, quad->color, sizeof(vert.color));
			array_push(ctx->vertex_buffer, vert);
		}
	}
}

uint16_t *txt_create_shared_index_buffer(size_t max_glyphs, size_t *size)
{
	static const uint16_t quad_index[] = { 
		0, 1, 2,
		1, 2, 3
	};

	*size = max_glyphs * sizeof(quad_index);
	uint16_t *buffer = malloc(*size);

	for (ufast32_t i = 0; i < max_glyphs; i++) {
		fast32_t offset = LENGTH(quad_index) * i;
		for (ufast32_t j = 0; j < LENGTH(quad_index); j++)
			buffer[offset+j] = 4*i + quad_index[j];
	}

	return buffer;
}

