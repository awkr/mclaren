#pragma once

#include "vk_context.h"
#include "vk_buffer.h"
#include <glm/glm.hpp>

// 顶点结构，手动构造 mesh 或加载 gltf/glb 模型时，顶点数据需遵循此结构
struct Vertex {
    alignas(16) float pos[3];
    alignas(16) float tex_coord[2];
    alignas(16) float normal[3];
    alignas(16) float color[4] = {1.0f, 1.0f, 1.0f, 1.0f}; // 默认白色
};

struct MeshBuffer {
    Buffer vertex_buffer;
    Buffer index_buffer;
    VkDeviceAddress vertex_buffer_device_address;
};

void create_mesh_buffer(VkContext *vk_context, const void *vertices, uint32_t vertex_count, uint32_t vertex_stride,
                        const void *indices, uint32_t index_count, uint32_t index_stride, MeshBuffer *mesh_buffer);

void destroy_mesh_buffer(VkContext *vk_context, MeshBuffer *mesh_buffer);
