#pragma once

#include "engine/mem/mem.h"
#define ARR_malloc  mem_malloc
#define ARR_free    mem_free
#define ARR_realloc mem_resize

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "engine/engine.h"
#include "engine/log/log.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define LENGTH(arr) (sizeof(arr)/sizeof(*arr))

#define CGLM_FORCE_DEPTH_ZERO_TO_ONE

typedef uint_fast32_t ufast32_t;
typedef  int_fast32_t  fast32_t;

typedef      uint32_t    Handle;

