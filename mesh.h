#pragma once

#include "mesh_buffer.h"

// 顶点结构，手动构造 mesh 或加载 gltf/glb 模型时，顶点数据需遵循此结构
struct Vertex {
    alignas(16) float pos[3];
    alignas(16) float tex_coord[2];
    alignas(16) float normal[3];
};

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

void destroy_mesh(VkContext *vk_context, Mesh *mesh);
