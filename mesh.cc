#include "mesh.h"
#include "vk_command_buffer.h"

void create_mesh(VkContext *vk_context, const void *vertices, uint32_t vertex_count, uint32_t vertex_stride,
                 const uint32_t *indices, uint32_t index_count, uint32_t index_stride, Mesh *mesh) {
    // todo 一次性上传顶点和索引数据

    { // vertex buffer
        const size_t vertex_buffer_size = vertex_count * vertex_stride;
        vk_create_buffer(vk_context, vertex_buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, &mesh->vertex_buffer);

        VkBufferDeviceAddressInfo buffer_device_address_info{};
        buffer_device_address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        buffer_device_address_info.buffer = mesh->vertex_buffer->handle;
        mesh->vertex_buffer_device_address = vkGetBufferDeviceAddress(vk_context->device, &buffer_device_address_info);

        // upload data to gpu
        Buffer *staging_buffer = nullptr;
        vk_create_buffer(vk_context, vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, &staging_buffer);

        VmaAllocationInfo staging_buffer_allocation_info;
        vmaGetAllocationInfo(vk_context->allocator, staging_buffer->allocation, &staging_buffer_allocation_info);

        memcpy(staging_buffer_allocation_info.pMappedData, vertices, vertex_buffer_size);
        vk_command_buffer_submit(vk_context, [&](VkCommandBuffer command_buffer) {
            vk_cmd_copy_buffer(command_buffer, staging_buffer->handle, mesh->vertex_buffer->handle, vertex_buffer_size, 0, 0);
        });
        vk_destroy_buffer(vk_context, staging_buffer);
    }

    if (index_count > 0) { // index buffer
        size_t index_buffer_size = index_count * index_stride;
        vk_create_buffer(vk_context, index_buffer_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY, &mesh->index_buffer);

        // upload data to gpu
        Buffer *staging_buffer = nullptr;
        vk_create_buffer(vk_context, index_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, &staging_buffer);

        VmaAllocationInfo staging_buffer_allocation_info;
        vmaGetAllocationInfo(vk_context->allocator, staging_buffer->allocation, &staging_buffer_allocation_info);

        memcpy(staging_buffer_allocation_info.pMappedData, indices, index_buffer_size);
        vk_command_buffer_submit(vk_context, [&](VkCommandBuffer command_buffer) {
            vk_cmd_copy_buffer(command_buffer, staging_buffer->handle, mesh->index_buffer->handle, index_buffer_size, 0, 0);
        });
        vk_destroy_buffer(vk_context, staging_buffer);
    }
}

void destroy_mesh(VkContext *vk_context, Mesh *mesh) {
    if (mesh->index_buffer) {
        vk_destroy_buffer(vk_context, mesh->index_buffer);
    }
    vk_destroy_buffer(vk_context, mesh->vertex_buffer);
}
