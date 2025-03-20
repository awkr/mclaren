#include "mesh.h"
#include "logging.h"
#include "mesh_system.h" // todo remove me
#include "vk_command_buffer.h"

void generate_aabb_from_vertices(const Vertex *vertices, uint32_t vertex_count, AABB *out_aabb) noexcept {
    ASSERT(vertex_count >= 2);
    for (size_t i = 0; i < vertex_count; ++i) {
        const glm::vec3 position = glm::vec3(vertices[i].position[0], vertices[i].position[1], vertices[i].position[2]);
        out_aabb->min = glm::min(out_aabb->min, position);
        out_aabb->max = glm::max(out_aabb->max, position);
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
