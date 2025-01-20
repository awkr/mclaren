#pragma once

#include "mesh.h"
#include <cstdint>

struct MeshSystemState {
  uint32_t mesh_id_counter = 0;
};

void create_mesh(MeshSystemState *mesh_system_state, VkContext *vk_context,
                 const void *vertices, uint32_t vertex_count, uint32_t vertex_stride,
                 const uint32_t *indices, uint32_t index_count, uint32_t index_stride,
                 Mesh *mesh);
void destroy_mesh(VkContext *vk_context, Mesh *mesh);
