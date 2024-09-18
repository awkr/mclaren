#pragma once

#include "geometry.h"
#include <cgltf.h>

void load_gltf(VkContext *vk_context, const char *filepath, Geometry *geometry);
