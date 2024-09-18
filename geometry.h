#pragma once

#include "mesh.h"

struct Geometry {
    std::vector<Mesh> meshes;
};

void create_geometry(VkContext *vk_context, const Vertex *vertices, uint32_t vertex_count, const uint32_t *indices, uint32_t index_count, Geometry *geometry);
void destroy_geometry(VkContext *vk_context, Geometry *geometry);
