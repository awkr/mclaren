#pragma once

#include "vk_context.h"
#include "vk_buffer.h"
#include <simd/simd.h>
#include <glm/glm.hpp>

struct MeshBuffer {
    Buffer vertex_buffer;
    Buffer index_buffer;
    VkDeviceAddress vertex_buffer_address;
};

struct MeshPushConstants {
    float model[16];
    VkDeviceAddress vertex_buffer_address;
};

void create_mesh_buffer(VkContext *vk_context, const void *vertices, uint32_t vertex_count, uint32_t vertex_stride,
                        const uint32_t *indices, uint32_t index_count, MeshBuffer *mesh_buffer);

void destroy_mesh_buffer(VkContext *vk_context, MeshBuffer *mesh_buffer);
