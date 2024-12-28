#include "gizmo.h"

void create_gizmo(MeshSystemState *mesh_system_state, VkContext *vk_context, const glm::vec3 &position, Gizmo *gizmo) {
  *gizmo = Gizmo(); // todo 改为memset
  gizmo->config.axis_length = 0.7f;
  gizmo->config.axis_radius = 0.004f;
  gizmo->config.arrow_length = 0.08f;
  gizmo->config.arrow_radius = 0.03f;
  gizmo->config.ring_major_radius = 0.4f;
  gizmo->config.ring_minor_radius = 0.004f;
  gizmo->config.cube_size = 0.05f;
  gizmo->config.cube_offset = 0.1f;
  gizmo->transform.position = position;
  gizmo->transform.scale = glm::vec3(1.0f, 1.0f, 1.0f);
  gizmo->rotation_start_pos = glm::vec3(FLT_MAX);
  gizmo->rotation_end_pos = glm::vec3(FLT_MAX);
  gizmo->rotation_clock_dir = '0';
}

void destroy_gizmo(Gizmo *gizmo, MeshSystemState *mesh_system_state, VkContext *vk_context) {
  destroy_geometry(mesh_system_state, vk_context, &gizmo->sector_geometry);
  destroy_geometry(mesh_system_state, vk_context, &gizmo->cube_geometry);
  destroy_geometry(mesh_system_state, vk_context, &gizmo->ring_geometry);
  destroy_geometry(mesh_system_state, vk_context, &gizmo->arrow_geometry);
  destroy_geometry(mesh_system_state, vk_context, &gizmo->axis_geometry);
}

void gizmo_set_position(Gizmo *gizmo, const glm::vec3 &position) {
  gizmo->transform.position = position;
}

void gizmo_set_scale(Gizmo *gizmo, const glm::vec3 &scale) {
  gizmo->transform.scale = scale;
}

const Transform &gizmo_get_transform(const Gizmo *gizmo) {
  return gizmo->transform;
}
