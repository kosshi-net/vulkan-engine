#include "common.h"

#include <ft2build.h>
#include FT_FREETYPE_H

static struct TextEngine {

	FT_Library ft;

} this;


int gfx_text_init()
{
	int ret;

	ret = FT_Init_FreeType(&this.ft);
	if(!ret) goto error;
	
	return 0;
error:
	return ret;
}


