#pragma once

#include "vk_context.h"

struct Buffer {
    VkBuffer handle;
    VkDeviceMemory device_memory;
};

void vk_create_buffer(VkContext *vk_context, size_t size, VkBufferUsageFlags buffer_usage, VkMemoryPropertyFlags memory_property_flags, Buffer **out_buffer);

void vk_destroy_buffer(VkContext *vk_context, Buffer *buffer);

void vk_alloc_buffer_memory(VkContext *vk_context, VkBuffer buffer, VkMemoryPropertyFlags memory_property_flags, VkDeviceMemory *memory);
void vk_copy_data_to_buffer(VkContext *vk_context, const void *data, size_t size, const Buffer *buffer);
void vk_clear_buffer(VkContext *vk_context, const Buffer *buffer, size_t size);

void vk_read_data_from_buffer(VkContext *vk_context, const Buffer *buffer, void *dst, size_t size);
