#include "vk_buffer.h"
#include "logging.h"

void vk_create_buffer(VkContext *vk_context, size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage, Buffer **out_buffer) {
    Buffer *buffer = new Buffer();

    VkBufferCreateInfo buffer_create_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_create_info.size = size;
    buffer_create_info.usage = usage;

    VmaAllocationCreateInfo allocation_create_info{};
    allocation_create_info.usage = memory_usage;
    allocation_create_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkResult result = vmaCreateBuffer(vk_context->allocator, &buffer_create_info, &allocation_create_info, &buffer->handle, &buffer->allocation, nullptr);
    ASSERT(result == VK_SUCCESS);

    *out_buffer = buffer;
}

void vk_destroy_buffer(VkContext *vk_context, Buffer *buffer) {
    vmaDestroyBuffer(vk_context->allocator, buffer->handle, buffer->allocation);
    delete buffer;
}

void vk_copy_data_to_buffer(VkContext *vk_context, const Buffer *buffer, const void *data, size_t size) {
    void *p = nullptr;
    VkResult result = vmaMapMemory(vk_context->allocator, buffer->allocation, &p);
    ASSERT(result == VK_SUCCESS);
    memcpy(p, data, size);
    vmaUnmapMemory(vk_context->allocator, buffer->allocation);
    result = vmaFlushAllocation(vk_context->allocator, buffer->allocation, 0, size);
    ASSERT(result == VK_SUCCESS);
}

void vk_read_data_from_buffer(VkContext *vk_context, const Buffer *buffer, void *dst, size_t size) {
  void *p = nullptr;
  const VkResult result = vmaMapMemory(vk_context->allocator, buffer->allocation, &p);
  ASSERT(result == VK_SUCCESS);
  memcpy(dst, p, size);
  vmaUnmapMemory(vk_context->allocator, buffer->allocation);
}
