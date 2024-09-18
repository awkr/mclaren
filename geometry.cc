#include "geometry.h"
#include <glm/gtc/constants.hpp>

void create_geometry(VkContext *vk_context, const Vertex *vertices, uint32_t vertex_count, const uint32_t *indices, uint32_t index_count, Geometry *geometry) {
    MeshBuffer mesh_buffer;
    create_mesh_buffer(vk_context, vertices, vertex_count, sizeof(Vertex), indices, index_count, sizeof(uint32_t), &mesh_buffer);

    Mesh mesh = {};
    mesh.mesh_buffer = mesh_buffer;

    Primitive primitive = {};
    primitive.index_count = index_count;
    primitive.index_offset = 0;
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

    create_geometry(vk_context, vertices, sizeof(vertices) / sizeof(Vertex), indices, index_count, geometry);
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

    vertices[8] = {{-0.5f, -0.5f,  -0.5f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}}; // left
    vertices[9] = {{-0.5f, -0.5f,   0.5f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}};
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

    create_geometry(vk_context, vertices, sizeof(vertices) / sizeof(Vertex), indices, index_count, geometry);
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

            const glm::vec3 &n = glm::normalize(glm::vec3(x, y, z));

            Vertex vertex = {};
            vertex.pos[0] = x;
            vertex.pos[1] = y;
            vertex.pos[2] = z;
            vertex.tex_coord[0] = (float) j / (float) sectors;
            vertex.tex_coord[1] = (float) i / (float) stacks;
            vertex.normal[0] = n.x;
            vertex.normal[1] = n.y;
            vertex.normal[2] = n.z;

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

    create_geometry(vk_context, vertices.data(), vertices.size(), indices.data(), indices.size(), geometry);
}

void create_cone_geometry(VkContext *vk_context, Geometry *geometry) {}
