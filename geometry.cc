#include "geometry.h"
#include "vk_command_buffer.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>

void create_geometry(VkContext *vk_context, const void *vertices, uint32_t vertex_count, uint32_t vertex_stride,
                     const uint32_t *indices, uint32_t index_count, uint32_t index_stride, Geometry *geometry) {
    Mesh mesh = {};

    // todo 一次性上传顶点和索引数据

    { // vertex buffer
        const size_t vertex_buffer_size = vertex_count * vertex_stride;
        vk_create_buffer(vk_context, vertex_buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, &mesh.vertex_buffer);

        VkBufferDeviceAddressInfo buffer_device_address_info{};
        buffer_device_address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        buffer_device_address_info.buffer = mesh.vertex_buffer->handle;
        mesh.vertex_buffer_device_address = vkGetBufferDeviceAddress(vk_context->device, &buffer_device_address_info);

        // upload data to gpu
        Buffer *staging_buffer = nullptr;
        vk_create_buffer(vk_context, vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, &staging_buffer);

        VmaAllocationInfo staging_buffer_allocation_info;
        vmaGetAllocationInfo(vk_context->allocator, staging_buffer->allocation, &staging_buffer_allocation_info);

        memcpy(staging_buffer_allocation_info.pMappedData, vertices, vertex_buffer_size);
        vk_command_buffer_submit(vk_context, [&](VkCommandBuffer command_buffer) {
            vk_command_copy_buffer(command_buffer, staging_buffer->handle, mesh.vertex_buffer->handle, vertex_buffer_size, 0, 0);
        });
        vk_destroy_buffer(vk_context, staging_buffer);
    }

    if (index_count > 0) { // index buffer
        size_t index_buffer_size = index_count * index_stride;
        vk_create_buffer(vk_context, index_buffer_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY, &mesh.index_buffer);

        // upload data to gpu
        Buffer *staging_buffer = nullptr;
        vk_create_buffer(vk_context, index_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, &staging_buffer);

        VmaAllocationInfo staging_buffer_allocation_info;
        vmaGetAllocationInfo(vk_context->allocator, staging_buffer->allocation, &staging_buffer_allocation_info);

        memcpy(staging_buffer_allocation_info.pMappedData, indices, index_buffer_size);
        vk_command_buffer_submit(vk_context, [&](VkCommandBuffer command_buffer) {
            vk_command_copy_buffer(command_buffer, staging_buffer->handle, mesh.index_buffer->handle, index_buffer_size, 0, 0);
        });
        vk_destroy_buffer(vk_context, staging_buffer);
    }

    Primitive primitive = {};
    primitive.index_offset = 0;
    primitive.index_count = index_count;
    primitive.vertex_offset = 0;
    primitive.vertex_count = vertex_count;

    mesh.primitives.push_back(primitive);

    geometry->meshes.push_back(mesh);
}

void destroy_geometry(VkContext *vk_context, Geometry *geometry) {
    for (Mesh &mesh : geometry->meshes) {
        destroy_mesh(vk_context, &mesh);
    }
}

void create_plane_geometry(VkContext *vk_context, float x, float y, Geometry *geometry) {
    Vertex vertices[4];
    // 2 -- 3
    // | \  |
    // |  \ |
    // 0 -- 1
    const float half_x = x / 2.0f;
    const float half_y = y / 2.0f;
    // clang-format off
    vertices[0] = {{-half_x, -half_y, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
    vertices[1] = {{ half_x, -half_y, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
    vertices[2] = {{-half_x,  half_y, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}};
    vertices[3] = {{ half_x,  half_y, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}};
    // clang-format on
    uint32_t indices[6] = {0, 1, 2, 2, 1, 3};
    uint32_t index_count = sizeof(indices) / sizeof(uint32_t);

    create_geometry(vk_context, vertices, sizeof(vertices) / sizeof(Vertex), sizeof(Vertex), indices, index_count, sizeof(uint32_t), geometry);
}

void create_cube_geometry(VkContext *vk_context, Geometry *geometry) {
    Vertex vertices[24];
    // clang-format off
    vertices[0] = {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f,  1.0f}}; // front
    vertices[1] = {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f,  1.0f}};
    vertices[2] = {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f,  1.0f}};
    vertices[3] = {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f,  1.0f}};

    vertices[4] = {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}}; // back
    vertices[5] = {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}};
    vertices[6] = {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}};
    vertices[7] = {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}};

    vertices[ 8] = {{-0.5f, -0.5f,  -0.5f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}}; // left
    vertices[ 9] = {{-0.5f, -0.5f,   0.5f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}};
    vertices[10] = {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}};
    vertices[11] = {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}};

    vertices[12] = {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}}; // right
    vertices[13] = {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
    vertices[14] = {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}};
    vertices[15] = {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}};

    vertices[16] = {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}}; // top
    vertices[17] = {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    vertices[18] = {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}};
    vertices[19] = {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}};

    vertices[20] = {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}}; // bottom
    vertices[21] = {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
    vertices[22] = {{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}};
    vertices[23] = {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}};

    uint32_t indices[36] = { 0,  1,  2,  2,  1,  3, // front
                             4,  5,  6,  6,  5,  7, // back
                             8,  9, 10, 10,  9, 11, // left
                            12, 13, 14, 14, 13, 15, // right
                            16, 17, 18, 18, 17, 19,
                            20, 21, 22, 22, 21, 23,};
    // clang-format on
    uint32_t index_count = sizeof(indices) / sizeof(uint32_t);

    create_geometry(vk_context, vertices, sizeof(vertices) / sizeof(Vertex), sizeof(Vertex), indices, index_count, sizeof(uint32_t), geometry);
}

void create_uv_sphere_geometry(VkContext *vk_context, float radius, uint16_t sectors, uint16_t stacks, Geometry *geometry) {
    assert(sectors < UINT16_MAX && stacks < UINT16_MAX);

    std::vector<Vertex> vertices;

    const float sector_steps = 2 * glm::pi<float>() / (float) sectors;
    const float stack_steps = glm::pi<float>() / (float) stacks;

    for (uint16_t i = 0; i <= stacks; ++i) {
        const float stack_angle = glm::pi<float>() / 2 - (float) i * stack_steps; // starting from pi/2 to -pi/2
        const float zx = radius * cosf(stack_angle);
        const float y = radius * sinf(stack_angle);

        // add (sectors + 1) vertices per stack,
        // first and last vertices have the same position and normal, but different tex coord
        for (uint16_t j = 0; j <= sectors; ++j) {
            const float sector_angle = (float) j * sector_steps; // starting from 0 to 2pi
            const float x = zx * sinf(sector_angle);
            const float z = zx * cosf(sector_angle);

            const glm::vec3 &pos = glm::vec3(x, y, z);
            const glm::vec3 &n = glm::normalize(pos);

            Vertex vertex = {};
            memcpy(vertex.pos, glm::value_ptr(pos), sizeof(float) * 3);
            vertex.tex_coord[0] = (float) j / (float) sectors;
            vertex.tex_coord[1] = (float) i / (float) stacks;
            memcpy(vertex.normal, glm::value_ptr(n), sizeof(float) * 3);

            vertices.push_back(vertex);
        }
    }

    // generate CCW index list of sphere triangles
    // a -- a+1
    // |  / |
    // | /  |
    // b -- b+1
    std::vector<uint32_t> indices;
    for (uint16_t i = 0; i <= stacks; ++i) {
        for (uint16_t j = 0; j <= sectors; ++j) {
            uint32_t a = i * (sectors + 1) + j;
            uint32_t b = a + sectors + 1;

            // 2 triangles per sector excluding first and last stacks
            if (i != 0) {
                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(a + 1);
            }
            if (i != (stacks - 1)) {
                indices.push_back(a + 1);
                indices.push_back(b);
                indices.push_back(b + 1);
            }
        }
    }

    create_geometry(vk_context, vertices.data(), vertices.size(), sizeof(Vertex), indices.data(), indices.size(), sizeof(uint32_t), geometry);
}

void create_ico_sphere_geometry(VkContext *vk_context, Geometry *geometry) {}

void create_cone_geometry(VkContext *vk_context, Geometry *geometry) {}
