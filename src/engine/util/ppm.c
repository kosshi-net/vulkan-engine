#include "util/ppm.h"
#include <stdio.h>

void gfx_util_write_ppm(
	const int width, 
	const int height, 
	unsigned char *buffer, 
	char *filename
){
	FILE *fp = fopen(filename, "wb"); /* b - binary mode */

	fprintf(fp, "P6\n%d %d\n255\n", width, height);

	for (int i = 0; i < width*height; ++i){
		static unsigned char color[3];
		color[0] = buffer[i];
		color[1] = buffer[i];
		color[2] = buffer[i];
		(void) fwrite(color, 1, 3, fp);
	}
	
	fclose(fp);
}

