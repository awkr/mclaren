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
