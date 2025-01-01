#pragma once

#include "vk_context.h"

struct Buffer {
    VkBuffer handle;
    VmaAllocation allocation;
};

void vk_create_buffer(VkContext *vk_context, size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage, Buffer **out_buffer);

void vk_destroy_buffer(VkContext *vk_context, Buffer *buffer);

void vk_copy_data_to_buffer(VkContext *vk_context, const Buffer *buffer, const void *data, size_t size);

void vk_read_data_from_buffer(VkContext *vk_context, const Buffer *buffer, void *dst, size_t size);
