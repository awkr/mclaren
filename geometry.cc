#include "geometry.h"

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

void create_quad_geometry(VkContext *vk_context, Geometry *geometry) {
    Vertex vertices[4];
    // clang-format off
    vertices[0] = {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    vertices[1] = {{ 0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    vertices[2] = {{-0.5f,  0.5f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    vertices[3] = {{ 0.5f,  0.5f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    // clang-format on
    uint32_t indices[6] = {0, 1, 2, 2, 1, 3};
    uint32_t index_count = sizeof(indices) / sizeof(uint32_t);

    create_geometry(vk_context, vertices, sizeof(vertices) / sizeof(Vertex), indices, index_count, geometry);
}

void create_cube_geometry(VkContext *vk_context, Geometry *geometry) {
    Vertex vertices[24];
    // clang-format off
    vertices[0] = {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f,  1.0f}, {1.0f, 0.0f, 0.0f, 0.5f}}; // front
    vertices[1] = {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f,  1.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    vertices[2] = {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f,  1.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    vertices[3] = {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f,  1.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};

    vertices[4] = {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f, 0.5f}}; // back
    vertices[5] = {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    vertices[6] = {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    vertices[7] = {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};

    vertices[8] = {{-0.5f, -0.5f,  -0.5f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}}; // left
    vertices[9] = {{-0.5f, -0.5f,   0.5f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    vertices[10] = {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    vertices[11] = {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};

    vertices[12] = {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}}; // right
    vertices[13] = {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    vertices[14] = {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    vertices[15] = {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};

    vertices[16] = {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}}; // top
    vertices[17] = {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    vertices[18] = {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    vertices[19] = {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};

    vertices[20] = {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}}; // bottom
    vertices[21] = {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    vertices[22] = {{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    vertices[23] = {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};

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

void create_sphere_geometry(VkContext *vk_context, Geometry *geometry) {
    Vertex vertices[24];
    // clang-format off
    vertices[0] = {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f,  1.0f}, {1.0f, 0.0f, 0.0f, 0.5f}}; // front
    vertices[1] = {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f,  1.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    vertices[2] = {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f,  1.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    vertices[3] = {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f,  1.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};

    vertices[4] = {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f, 0.5f}}; // back
    vertices[5] = {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    vertices[6] = {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    vertices[7] = {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};

    vertices[8] = {{-0.5f, -0.5f,  -0.5f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}}; // left
    vertices[9] = {{-0.5f, -0.5f,   0.5f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    vertices[10] = {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    vertices[11] = {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};

    vertices[12] = {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}}; // right
    vertices[13] = {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    vertices[14] = {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    vertices[15] = {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};

    vertices[16] = {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}}; // top
    vertices[17] = {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    vertices[18] = {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    vertices[19] = {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};

    vertices[20] = {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}}; // bottom
    vertices[21] = {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    vertices[22] = {{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};
    vertices[23] = {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}};

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

void create_circle_geometry(VkContext *vk_context, Geometry *geometry) {}

void create_cone_geometry(VkContext *vk_context, Geometry *geometry) {}
