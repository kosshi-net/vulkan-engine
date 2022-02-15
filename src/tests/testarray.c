#include "array.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

static const char teststring[] = 
	"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
	"tempor incididunt ut labore et dolore magna aliqua.";

int main(void)
{

	char *array = array_create(sizeof(char));

	const char *c = teststring;
	while(*c){
		array_push(&array, c++);
	}
	if( strcmp(teststring, array ) != 0){
		printf("Strings don't match\n");
		return 1;
	}

	printf("%s\n", array);
	array_destroy(&array);
	if(array != NULL){
		printf("Failure to NULL a variable\n");
		return 1;
	}
	printf("Pass char\n");

	int testsize = 1024*8;
	uint64_t testints[testsize];
	for(int i = 0; i < testsize; i++)
		testints[i] = rand();
	
	uint64_t *array2 = array_create(sizeof(uint64_t));
	for(int i = 0; i < testsize; i++)
		array_push(&array2, testints+i);
	

	for(int i = 0; i < testsize; i++){
		if( (array2)[i] != testints[i] ){
			printf("Ints don't match\n");
			return 1;
		}
	}
	array_destroy(&array2);
	printf("Pass ints\n");




	return 0;
}
