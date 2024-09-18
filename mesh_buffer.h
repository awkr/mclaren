#pragma once

#include "vk_context.h"
#include "vk_buffer.h"
#include <glm/glm.hpp>

struct MeshBuffer {
    Buffer vertex_buffer;
    Buffer index_buffer;
    VkDeviceAddress vertex_buffer_device_address;
};

void create_mesh_buffer(VkContext *vk_context, const void *vertices, uint32_t vertex_count, uint32_t vertex_stride,
                        const void *indices, uint32_t index_count, uint32_t index_stride, MeshBuffer *mesh_buffer);

void destroy_mesh_buffer(VkContext *vk_context, MeshBuffer *mesh_buffer);
