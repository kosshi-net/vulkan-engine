#include "text/text_engine.h"
#include "text/text_common.h"

#include "engine.h"
#include "log/log.h"
#include "util/ppm.h"
#include "handle/handle.h"

#include <errno.h>
#include <iconv.h>

#include <harfbuzz/hb-ft.h>
#include <GLFW/glfw3.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <fribidi.h>

static FT_Library ft;

static struct HandleAllocator alloc = HANDLE_ALLOCATOR(struct TextEngine, 1);

struct TextEngine *text_engine_get_struct(TextEngine handle)
{
	return handle_dereference(&alloc, handle);
}

void add_font(
	struct TextEngine *restrict this, 
	char *name, 
	uint32_t size,
	enum FontStyle style)
{
	log_debug("Loading font %s", name);

	struct Font *f = array_reserve(this->fonts, 1);
	f->style = style;

	uint32_t ret = FT_New_Face(ft,
		name,
		0,
		&f->ft_face
	);
	if(ret) engine_crash("FT_New_face failed");


	if (f->ft_face->num_fixed_sizes) {
		log_debug("%s fixed sizes:", name);
		for (fast32_t j = 0; j < f->ft_face->num_fixed_sizes; j++)
		{
			log_debug("%ix%i", 
				f->ft_face->available_sizes[j].width,
				f->ft_face->available_sizes[j].height
			);
		}
	}

	ret = FT_Set_Pixel_Sizes(f->ft_face, 0, size);
	if(ret) engine_crash("FT_Set_Pixel_Sizes failed");

	f->hb_font = hb_ft_font_create_referenced(f->ft_face);
	hb_ft_font_set_funcs(f->hb_font);

	this->font_count++;
}

void create_bin( 
	struct TextEngine *this, 
	uint32_t bin_num, 
	uint32_t slot_rows,
	uint32_t slot_w,
	uint32_t slot_h
){
	struct Atlas    *restrict atlas = &this->atlas;
	struct AtlasBin *restrict bin   = &this->atlas.bin[bin_num];

	int atlas_y = 0;
	if (bin_num) 
		atlas_y = bin[-1].atlas_y + bin[-1].atlas_h;

	bin->atlas_y = atlas_y;
	bin->w = slot_w;
	bin->h = slot_h;
	bin->atlas_h = slot_rows * slot_h;

	uint32_t slot_cols = (atlas->w / bin->w);
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

TextEngine text_engine_create(void)
{
	TextEngine handle = handle_allocate(&alloc);

	struct TextEngine *restrict this = text_engine_get_struct(handle);

	uint32_t ret; 

	this->font_size = 16;
	array_create(this->fonts);

	if (!ft) {
		ret = FT_Init_FreeType(&ft);
		if(ret) 
			engine_crash("FT_Init_FreeType failed");
	}

	add_font(this, "fonts/TerminusTTF.ttf",         
		16, FONT_STYLE_MONOSPACE);
	add_font(this, "fonts/sazanami-gothic.ttf", 
		16, FONT_STYLE_MONOSPACE);
	add_font(this, "fonts/NotoSans-Bold.ttf", 
		this->font_size, FONT_STYLE_BOLD);
	add_font(this, "fonts/NotoSans-Italic.ttf", 
		this->font_size, FONT_STYLE_ITALIC);
	add_font(this, "fonts/NotoSans-BoldItalic.ttf",
		this->font_size, FONT_STYLE_BOLD | FONT_STYLE_ITALIC);

	add_font(this, "fonts/NotoSans-Regular.ttf", 
		this->font_size, FONT_STYLE_NORMAL);
	add_font(this, "fonts/NotoSansCJK-Medium.ttc",
		this->font_size, FONT_STYLE_NORMAL);
	add_font(this, "fonts/NotoSansArabic-Regular.ttf",
		this->font_size, FONT_STYLE_NORMAL);
	add_font(this, "fonts/NotoSansHebrew-Regular.ttf",
		this->font_size, FONT_STYLE_NORMAL);


	memset(this->style.color, 0xFF, LENGTH(this->style.color));

	/* ATLAS */
	
	struct Atlas *atlas = &this->atlas;

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

	create_bin(
		this, bin++,
		4 * pages_small,
		page/4, page/4
	);

	create_bin(
		this, bin++,
		2 * (pages - pages_small - pages_big),
		page/2, page/2
	);

	create_bin(
		this, bin++,
		1 * pages_big,
		page, page
	);

	return handle;
}


struct GlyphSlot *text_engine_find_glyph(
	TextEngine handle,
	uint32_t   font, 
	uint32_t   code)
{
	struct TextEngine *restrict this = text_engine_get_struct(handle);

	for (uint32_t bin  = 0; bin  < this->atlas.bin_count;           bin++)
	for (uint32_t s    = 0; s    < this->atlas.bin[bin].slot_count; s++) {
		struct GlyphSlot *slot =  &this->atlas.bin[bin].slot[s];
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

struct GlyphSlot *text_engine_cache_glyph( 
	TextEngine handle,
	uint32_t font_id,
	uint32_t code)
{
	struct TextEngine *restrict this = handle_dereference(&alloc, handle);

	struct Atlas    *atlas = &this->atlas;
	struct AtlasBin *bin;
	struct GlyphSlot *slot;
	struct Font *font = this->fonts+font_id;
	
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
	if (!this->mute_logging) 
		log_error("No bin big enough! %i %i",w,h);
	goto error;
bin_found:
	/* Use any free slot first, evict refcount=0 from cache as last resort */
	bin = &this->atlas.bin[bin_num];
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
	if (!this->mute_logging) 
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
		if (!this->mute_logging) 
			log_error("Unsupported pixel format %i", glyph->format);
		goto error;
	}

	slot->valid = true;
	atlas->dirty = 1;

	return slot;
error:
	if (!this->mute_logging) 
		log_warn("Further logs for this text context will be muted");
	this->mute_logging = true;
	return NULL;
}


void text_engine_export_atlas(TextEngine handle)
{
	struct TextEngine *restrict this = text_engine_get_struct(handle);
	log_info("Exported atlas to atlas.ppm");
	gfx_util_write_ppm(this->atlas.w, this->atlas.h, this->atlas.bitmap, "atlas.ppm");
}

