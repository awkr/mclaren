#pragma once

#include "mesh.h"

struct Geometry {
    std::vector<Mesh> meshes;
};

// create a geometry with a single mesh which contains only a primtive
void create_geometry(VkContext *vk_context, const void *vertices, uint32_t vertex_count, uint32_t vertex_stride,
                     const uint32_t *indices, uint32_t index_count, uint32_t index_stride, Geometry *geometry);
void destroy_geometry(VkContext *vk_context, Geometry *geometry);

void create_plane_geometry(VkContext *vk_context, float x, float y, Geometry *geometry);
void create_cube_geometry(VkContext *vk_context, Geometry *geometry);
void create_uv_sphere_geometry(VkContext *vk_context, float radius, uint16_t sectors, uint16_t stacks, Geometry *geometry);
void create_ico_sphere_geometry(VkContext *vk_context, Geometry *geometry);
void create_cone_geometry(VkContext *vk_context, Geometry *geometry);
