#pragma once

#include "vk_context.h"

struct Buffer {
    VkBuffer handle;
    VmaAllocation allocation;
};

void vk_create_buffer(VkContext *vk_context, size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage,
                      Buffer *buffer);

void vk_destroy_buffer(VkContext *vk_context, Buffer *buffer);