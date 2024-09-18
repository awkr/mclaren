#include "mesh.h"

void destroy_mesh(VkContext *vk_context, Mesh *mesh) { destroy_mesh_buffer(vk_context, &mesh->mesh_buffer); }
