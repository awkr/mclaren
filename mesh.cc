#include "mesh.h"

void destroy_mesh(VkContext *vk_context, Mesh *mesh) {
    if (mesh->index_buffer) {
        vk_destroy_buffer(vk_context, mesh->index_buffer);
    }
    vk_destroy_buffer(vk_context, mesh->vertex_buffer);
}
