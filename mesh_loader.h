#pragma once

#include "geometry.h"
#include <cgltf.h>

void load_gltf(MeshSystemState *mesh_system_state, VkContext *vk_context, const char *filepath, Geometry *geometry);
