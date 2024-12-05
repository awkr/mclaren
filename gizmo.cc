#include "gizmo.h"

void create_gizmo(const glm::vec3 &position, Gizmo *gizmo) {
  gizmo->config.axis_length = 0.7f;
  gizmo->config.axis_radius = 0.008f;
  gizmo->config.arrow_length = 0.08f;
  gizmo->config.arrow_radius = 0.04f;
  gizmo->config.ring_major_radius = 0.4f;
  gizmo->config.ring_minor_radius = 0.008f;
  gizmo->transform.position = position;
  gizmo->transform.scale = glm::vec3(1.0f, 1.0f, 1.0f);
}

void destroy_gizmo(Gizmo *gizmo, MeshSystemState *mesh_system_state, VkContext *vk_context) {
  destroy_geometry(mesh_system_state, vk_context, &gizmo->cube_geometry);
  destroy_geometry(mesh_system_state, vk_context, &gizmo->ring_geometry);
  destroy_geometry(mesh_system_state, vk_context, &gizmo->arrow_geometry);
  destroy_geometry(mesh_system_state, vk_context, &gizmo->axis_geometry);
}

void gizmo_set_position(Gizmo *gizmo, const glm::vec3 &position) {
  gizmo->transform.position = position;
}

const Transform &gizmo_get_transform(const Gizmo *gizmo) {
  return gizmo->transform;
}
