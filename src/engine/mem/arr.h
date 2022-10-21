#pragma once

#define arr_size(arr)\
	(sizeof(*arr) * arr##_count)

#define arr_push( arr, ... )                    \
{                                               \
	size_t _size = mem_size(arr);               \
	if ( arr_size(arr) >= _size) {              \
		arr = mem_resize(arr, _size * 2);       \
	}                                           \
	arr[(arr##_count)++] = __VA_ARGS__;                 \
}


#define arr_reserve(arr, n)                        \
{                                                  \
	arr##count += n;                               \
	while (arr_size(arr) > mem_size(arr)) {        \
		arr = mem_resize(arr, mem_size(arr)*2);    \
	}                                              \
}
