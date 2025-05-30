#include "vk_buffer.h"
#include "core/logging.h"

void vk_create_buffer(VkContext *vk_context, size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage, Buffer *buffer) {
    VkBufferCreateInfo buffer_create_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_create_info.size = size;
    buffer_create_info.usage = usage;

    VmaAllocationCreateInfo allocation_create_info{};
    allocation_create_info.usage = memory_usage;
    allocation_create_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkResult result = vmaCreateBuffer(vk_context->allocator, &buffer_create_info, &allocation_create_info,
                                      &buffer->handle, &buffer->allocation, nullptr);
    ASSERT(result == VK_SUCCESS);
}

void vk_destroy_buffer(VkContext *vk_context, Buffer *buffer) {
    vmaDestroyBuffer(vk_context->allocator, buffer->handle, buffer->allocation);
}

void vk_copy_data_to_buffer(VkContext *vk_context, const Buffer *buffer, const void *data, size_t size) {
    void *mapped_ptr = nullptr;
    VkResult result = vmaMapMemory(vk_context->allocator, buffer->allocation, &mapped_ptr);
    ASSERT(result == VK_SUCCESS);
    memcpy(mapped_ptr, data, size);
    vmaUnmapMemory(vk_context->allocator, buffer->allocation);
}
