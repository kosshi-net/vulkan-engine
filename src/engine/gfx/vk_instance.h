#pragma once

#include "common.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


/**************
 * EXTENSIONS *
 **************/

bool vk_instance_ext_check(const char*);
void vk_instance_ext_get_avbl(void);
void vk_instance_ext_add(const char*);

bool vk_validation_check(const char*);
void vk_validation_get_avbl(void);
void vk_validation_add(const char*);

void vk_create_instance(void);
