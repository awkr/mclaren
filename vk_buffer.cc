#include "vk_buffer.h"
#include "logging.h"

void vk_create_buffer_vma(VkContext *vk_context, size_t size, VkBufferUsageFlags buffer_usage, VmaMemoryUsage memory_usage, VmaAllocationCreateFlags flag, Buffer **out_buffer) {
    Buffer *buffer = new Buffer();

    VkBufferCreateInfo buffer_create_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_create_info.size = size;
    buffer_create_info.usage = buffer_usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocation_create_info{};
    allocation_create_info.usage = memory_usage;
    allocation_create_info.flags = flag;

    VkResult result = vmaCreateBuffer(vk_context->allocator, &buffer_create_info, &allocation_create_info, &buffer->handle, &buffer->allocation, nullptr);
    ASSERT(result == VK_SUCCESS);

    *out_buffer = buffer;
}

void vk_create_buffer(VkContext *vk_context, size_t size, VkBufferUsageFlags buffer_usage, VkMemoryPropertyFlags memory_property_flags, Buffer **out_buffer) {
    Buffer *buffer = new Buffer();

    VkBufferCreateInfo buffer_create_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_create_info.size = size;
    buffer_create_info.usage = buffer_usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(vk_context->device, &buffer_create_info, nullptr, &buffer->handle);
    ASSERT(result == VK_SUCCESS);

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(vk_context->device, buffer->handle, &memory_requirements);

    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(vk_context->physical_device, &memory_properties);

    uint32_t memory_type_index = UINT32_MAX;
    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i) {
      if ((memory_requirements.memoryTypeBits & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & memory_property_flags)) {
        memory_type_index = i;
        break;
      }
    }
    ASSERT(memory_type_index != UINT32_MAX);

    VkMemoryAllocateFlagsInfo allocate_flags_info{};
    allocate_flags_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    if (buffer_usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
      allocate_flags_info.flags |= VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
    }

    VkMemoryAllocateInfo memory_allocate_info{};
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.allocationSize = memory_requirements.size;
    memory_allocate_info.memoryTypeIndex = memory_type_index;
    if (buffer_usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
      memory_allocate_info.pNext = &allocate_flags_info;
    }

    result = vkAllocateMemory(vk_context->device, &memory_allocate_info, nullptr, &buffer->device_memory);
    ASSERT(result == VK_SUCCESS);

    result = vkBindBufferMemory(vk_context->device, buffer->handle, buffer->device_memory, 0);
    ASSERT(result == VK_SUCCESS);

    *out_buffer = buffer;
}

void vk_destroy_buffer_vma(VkContext *vk_context, Buffer *buffer) {
    vmaDestroyBuffer(vk_context->allocator, buffer->handle, buffer->allocation);
    delete buffer;
}

void vk_destroy_buffer(VkContext *vk_context, Buffer *buffer) {
    vkDestroyBuffer(vk_context->device, buffer->handle, nullptr);
    vkFreeMemory(vk_context->device, buffer->device_memory, nullptr);
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

void vk_clear_buffer(VkContext *vk_context, const Buffer *buffer, size_t size) {
  void *p = nullptr;
  VkResult result = vmaMapMemory(vk_context->allocator, buffer->allocation, &p);
  ASSERT(result == VK_SUCCESS);
  memset(p, 0, size);
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
