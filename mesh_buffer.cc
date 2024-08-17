#include "mesh_buffer.h"
#include "vk_command_buffer.h"
#include "vk_queue.h"
#include "vk_fence.h"

void create_mesh_buffer(VkContext *vk_context, const void *vertices, uint32_t vertex_count, uint32_t vertex_stride,
                        const uint32_t *indices, uint32_t index_count, MeshBuffer *mesh_buffer) {
    const size_t vertex_buffer_size = vertex_count * vertex_stride;
    const size_t index_buffer_size = index_count * sizeof(uint32_t);

    vk_create_buffer(vk_context, vertex_buffer_size,
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY, &mesh_buffer->vertex_buffer);

    VkBufferDeviceAddressInfo buffer_device_address_info{};
    buffer_device_address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    buffer_device_address_info.buffer = mesh_buffer->vertex_buffer.handle;
    mesh_buffer->vertex_buffer_address = vkGetBufferDeviceAddress(vk_context->device, &buffer_device_address_info);

    vk_create_buffer(vk_context, index_buffer_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VMA_MEMORY_USAGE_GPU_ONLY, &mesh_buffer->index_buffer);

    Buffer staging_buffer;
    vk_create_buffer(vk_context, vertex_buffer_size + index_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VMA_MEMORY_USAGE_CPU_ONLY, &staging_buffer);

    VmaAllocationInfo staging_buffer_allocation_info;
    vmaGetAllocationInfo(vk_context->allocator, staging_buffer.allocation, &staging_buffer_allocation_info);

    void *dst = staging_buffer_allocation_info.pMappedData;
    memcpy(dst, vertices, vertex_buffer_size);
    memcpy((void *) ((uintptr_t) dst + vertex_buffer_size), indices, index_buffer_size);

    VkCommandBuffer command_buffer;
    vk_alloc_command_buffers(vk_context->device, vk_context->command_pool, 1, &command_buffer);
    vk_begin_one_flight_command_buffer(command_buffer);

    vk_command_copy_buffer(command_buffer, staging_buffer.handle, mesh_buffer->vertex_buffer.handle,
                           vertex_buffer_size, 0, 0);
    vk_command_copy_buffer(command_buffer, staging_buffer.handle, mesh_buffer->index_buffer.handle, index_buffer_size,
                           vertex_buffer_size, 0);

    vk_end_command_buffer(command_buffer);

    VkFence fence;
    vk_create_fence(vk_context->device, 0, &fence);

    vk_queue_submit(vk_context->graphics_queue, command_buffer, fence);

    vk_wait_fence(vk_context->device, fence); // make sure data is copied before destroying staging buffer
    vk_destroy_fence(vk_context->device, fence);

    vk_destroy_buffer(vk_context, &staging_buffer);
}

void destroy_mesh_buffer(VkContext *vk_context, MeshBuffer *mesh_buffer) {
    vk_destroy_buffer(vk_context, &mesh_buffer->index_buffer);
    vk_destroy_buffer(vk_context, &mesh_buffer->vertex_buffer);
}
