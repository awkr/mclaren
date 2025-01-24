#include "mesh_system.h"
#include "logging.h"
#include "vk_command_buffer.h"

void create_mesh(MeshSystemState *mesh_system_state, VkContext *vk_context,
                 const void *vertices, uint32_t vertex_count, uint32_t vertex_stride,
                 const uint32_t *indices, uint32_t index_count, uint32_t index_stride,
                 Mesh *mesh) {
  const size_t vertex_buffer_size = vertex_count * vertex_stride;
  const size_t index_buffer_size = index_count * index_stride;
  vk_create_buffer(vk_context, vertex_buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &mesh->vertex_buffer);
  if (index_count > 0) {
    vk_create_buffer(vk_context, index_buffer_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &mesh->index_buffer);
  }

  VkBufferDeviceAddressInfo buffer_device_address_info{};
  buffer_device_address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  buffer_device_address_info.buffer = mesh->vertex_buffer->handle;
  mesh->vertex_buffer_device_address = vkGetBufferDeviceAddress(vk_context->device, &buffer_device_address_info);

  Buffer *vertices_staging_buffer = nullptr;
  Buffer *indices_staging_buffer = nullptr;
  vk_create_buffer(vk_context, vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertices_staging_buffer);
  {
    void *data = nullptr;
    VkResult result = vkMapMemory(vk_context->device, vertices_staging_buffer->device_memory, 0, vertex_buffer_size, 0, &data);
    ASSERT(result == VK_SUCCESS);
    memcpy(data, vertices, vertex_buffer_size);
    vkUnmapMemory(vk_context->device, vertices_staging_buffer->device_memory);
  }
  if (index_count > 0) {
    vk_create_buffer(vk_context, index_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &indices_staging_buffer);
    {
      void *data = nullptr;
      VkResult result = vkMapMemory(vk_context->device, indices_staging_buffer->device_memory, 0, index_buffer_size, 0, &data);
      ASSERT(result == VK_SUCCESS);
      memcpy(data, indices, index_buffer_size);
      vkUnmapMemory(vk_context->device, indices_staging_buffer->device_memory);
    }
  }

  vk_command_buffer_submit(vk_context, [&](VkCommandBuffer command_buffer) {
    vk_cmd_copy_buffer2(command_buffer, vertices_staging_buffer->handle, mesh->vertex_buffer->handle, vertex_buffer_size, 0, 0);
    if (index_count > 0) {
      vk_cmd_copy_buffer2(command_buffer, indices_staging_buffer->handle, mesh->index_buffer->handle, index_buffer_size, 0, 0);
    }
  }, vk_context->graphics_queue);
  vk_destroy_buffer(vk_context, vertices_staging_buffer);
  if (index_count > 0) {
    vk_destroy_buffer(vk_context, indices_staging_buffer);
  }

  mesh->id = ++mesh_system_state->mesh_id_counter;
}

void destroy_mesh(VkContext *vk_context, Mesh *mesh) {
  if (mesh->index_buffer) {
    vk_destroy_buffer(vk_context, mesh->index_buffer);
  }
  vk_destroy_buffer(vk_context, mesh->vertex_buffer);
}
