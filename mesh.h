#pragma once

#include "vk_buffer.h"
#include <glm/glm.hpp>

// 顶点结构，手动构造 mesh 或加载 gltf/glb 模型时，顶点数据需遵循此结构
struct alignas(16) Vertex {
    alignas(16) float pos[3];
    alignas(16) float tex_coord[2];
    alignas(16) float normal[3];
};

struct alignas(16) ColoredVertex {
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec4 color;
};

// 可被渲染的最小粒度的实体，若该实体支持 index，则调用 vkCmdDrawIndexed，否则调用 vkCmdDraw
struct Primitive {
    uint32_t index_offset;
    uint32_t index_count;
    uint32_t vertex_offset;
    uint32_t vertex_count;
};

struct Mesh {
    uint32_t id;
    std::vector<Primitive> primitives;

    Buffer *vertex_buffer;
    Buffer *index_buffer;
    VkDeviceAddress vertex_buffer_device_address;

    // glm::vec3 translation;
    // glm::vec3 euler_angles; // in degrees, not radians
    // glm::vec3 scale;
};

void destroy_mesh(VkContext *vk_context, Mesh *mesh);
