#include "fileutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

char *read_file(const char *path, size_t *size){
	FILE *f = fopen(path, "r");
	if(f==NULL) return NULL;

	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET); 

	char *string = calloc(fsize, 1 );
	if(string==NULL) return NULL;

	fread(string, 1, fsize, f);
	fclose(f);

	if(size != NULL)
		*size = fsize;

	return string;
}


