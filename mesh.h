#pragma once

#include "vk_buffer.h"
#include <glm/glm.hpp>

// 顶点结构，手动构造 mesh 或加载 gltf/glb 模型时，顶点数据需遵循此结构
struct alignas(16) Vertex {
    alignas(16) float pos[3];
    alignas(16) float tex_coord[2];
    alignas(16) float normal[3];
};

// 顶点结构，一般用于 debug draw，如 bounding box，坐标轴、射线等
struct alignas(16) ColoredVertex {
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec4 color;
};

// 可被渲染的最小粒度的实体，若 `index_count` 大于 0，则调用 vkCmdDrawIndexed，否则调用 vkCmdDraw
struct Primitive {
    uint32_t index_offset;
    uint32_t index_count;
    uint32_t vertex_offset;
    uint32_t vertex_count;
};

struct BoundingBox {
    glm::vec3 min = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    glm::vec3 max = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
};

struct Mesh {
    uint32_t id;

    Buffer *vertex_buffer;
    Buffer *index_buffer;
    VkDeviceAddress vertex_buffer_device_address;

    std::vector<Primitive> primitives;
    BoundingBox bounding_box;

    // glm::vec3 translation;
    // glm::vec3 euler_angles; // in degrees, not radians
    // glm::vec3 scale;
};

void create_mesh(VkContext *vk_context, const void *vertices, uint32_t vertex_count, uint32_t vertex_stride, const uint32_t *indices, uint32_t index_count, uint32_t index_stride, Mesh *mesh);
void destroy_mesh(VkContext *vk_context, Mesh *mesh);
