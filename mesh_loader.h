#pragma once

#include "mesh_buffer.h"
#include <cgltf.h>

struct Primitive {
    uint32_t index_offset;
    uint32_t index_count;
};

struct Mesh {
    uint32_t id;
    std::vector<Primitive> primitives;
    MeshBuffer mesh_buffer;

    glm::vec3 translation;
    glm::vec3 euler_angles; // in degrees, not radians
    glm::vec3 scale;
};

struct Geometry {
    std::vector<Mesh> meshes;
};

void load_gltf(VkContext *vk_context, const char *filepath, Geometry *geometry);

void destroy_geometry(VkContext *vk_context, Geometry *geometry);

void destroy_mesh(VkContext *vk_context, Mesh *mesh);
