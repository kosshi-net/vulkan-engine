#include "res/res.h"
#include "common.h"
#include "util/file.h"

static const char * const resource_paths[] = {
	[RES_NONE]              =  NULL,
	[RES_TEAPOT]            = "teapot.obj",
	[RES_TEXTURE_TEST]      = "texture.png",

	[RES_SHADER_FRAG_TEST]  = "frag.spv",
	[RES_SHADER_VERT_TEST]  = "vert.spv",

	[RES_SHADER_FRAG_TEXT]  = "text.frag.spv",
	[RES_SHADER_VERT_TEXT]  = "text.vert.spv",

	[RES_SHADER_FRAG_WIDGET]  = "widget.frag.spv",
	[RES_SHADER_VERT_WIDGET]  = "widget.vert.spv",

	[RES_SHADER_FRAG_SOFTBODY]  = "softbody.frag.spv",
	[RES_SHADER_VERT_SOFTBODY]  = "softbody.vert.spv",
	[RES_SHADER_VERT_SOFTBODY_SHADOW]  = "softbody.shadow.vert.spv",

	[RES_SHADER_FRAG_WIREFRAME]  = "wireframe.frag.spv",
	[RES_SHADER_VERT_WIREFRAME]  = "wireframe.vert.spv",

	[RES_SHADER_FRAG_SPHERE]  = "sphere.frag.spv",
	[RES_SHADER_VERT_SPHERE]  = "sphere.vert.spv",
};

void *res_file(enum Resource res, size_t *size)
{
	const char *path = resource_paths[res];
	if( path == NULL ){
		return NULL;
	}

	char *file = read_file(path, size);
	return file;
}


