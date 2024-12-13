#include "mesh.h"
#include "core/logging.h"
#include "vk_command_buffer.h"

void generate_aabb_from_vertices(const Vertex *vertices, uint32_t vertex_count, AABB *out_aabb) noexcept {
    ASSERT(vertex_count >= 2);
    for (size_t i = 0; i < vertex_count; ++i) {
        const glm::vec3 position = glm::vec3(vertices[i].position[0], vertices[i].position[1], vertices[i].position[2]);
        out_aabb->min = glm::min(out_aabb->min, position);
        out_aabb->max = glm::max(out_aabb->max, position);
    }
}

void create_mesh(MeshSystemState *mesh_system_state, VkContext *vk_context, const void *vertices, uint32_t vertex_count, uint32_t vertex_stride, const uint32_t *indices, uint32_t index_count, uint32_t index_stride, Mesh *mesh) {
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

  mesh->id = ++mesh_system_state->mesh_id_generator;
}

void destroy_mesh(VkContext *vk_context, Mesh *mesh) {
    if (mesh->index_buffer) {
        vk_destroy_buffer(vk_context, mesh->index_buffer);
    }
  if (mesh->vertex_buffer) { // todo 移除该判断
    vk_destroy_buffer(vk_context, mesh->vertex_buffer);
  }
}

void create_mesh_from_aabb(MeshSystemState *mesh_system_state, VkContext *vk_context, const AABB &aabb, Mesh &mesh) {
    Vertex vertices[24];

    // bottom
    vertices[0] = {{aabb.min.x, aabb.min.y, aabb.min.z}, {}, {}};
    vertices[1] = {{aabb.max.x, aabb.min.y, aabb.min.z}, {}, {}};
    vertices[2] = {{aabb.min.x, aabb.min.y, aabb.max.z}, {}, {}};
    vertices[3] = {{aabb.max.x, aabb.min.y, aabb.max.z}, {}, {}};
    vertices[4] = {{aabb.min.x, aabb.min.y, aabb.min.z}, {}, {}};
    vertices[5] = {{aabb.min.x, aabb.min.y, aabb.max.z}, {}, {}};
    vertices[6] = {{aabb.max.x, aabb.min.y, aabb.min.z}, {}, {}};
    vertices[7] = {{aabb.max.x, aabb.min.y, aabb.max.z}, {}, {}};

    // side
    vertices[8] = {{aabb.min.x, aabb.min.y, aabb.min.z}, {}, {}};
    vertices[9] = {{aabb.min.x, aabb.max.y, aabb.min.z}, {}, {}};
    vertices[10] = {{aabb.max.x, aabb.min.y, aabb.min.z}, {}, {}};
    vertices[11] = {{aabb.max.x, aabb.max.y, aabb.min.z}, {}, {}};
    vertices[12] = {{aabb.min.x, aabb.min.y, aabb.max.z}, {}, {}};
    vertices[13] = {{aabb.min.x, aabb.max.y, aabb.max.z}, {}, {}};
    vertices[14] = {{aabb.max.x, aabb.min.y, aabb.max.z}, {}, {}};
    vertices[15] = {{aabb.max.x, aabb.max.y, aabb.max.z}, {}, {}};

    // top
    vertices[16] = {{aabb.min.x, aabb.max.y, aabb.min.z}, {}, {}};
    vertices[17] = {{aabb.max.x, aabb.max.y, aabb.min.z}, {}, {}};
    vertices[18] = {{aabb.min.x, aabb.max.y, aabb.max.z}, {}, {}};
    vertices[19] = {{aabb.max.x, aabb.max.y, aabb.max.z}, {}, {}};
    vertices[20] = {{aabb.min.x, aabb.max.y, aabb.min.z}, {}, {}};
    vertices[21] = {{aabb.min.x, aabb.max.y, aabb.max.z}, {}, {}};
    vertices[22] = {{aabb.max.x, aabb.max.y, aabb.min.z}, {}, {}};
    vertices[23] = {{aabb.max.x, aabb.max.y, aabb.max.z}, {}, {}};

    create_mesh(mesh_system_state, vk_context, vertices, 24, sizeof(Vertex), nullptr, 0, 0, &mesh);

    Primitive primitive{};
    primitive.vertex_offset = 0;
    primitive.vertex_count = 24;
    primitive.index_offset = 0;
    primitive.index_count = 0;

    mesh.primitives.push_back(primitive);
}
