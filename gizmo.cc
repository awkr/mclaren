#include "gizmo.h"

void create_gizmo(const glm::vec3 &position, Gizmo *gizmo) {
  memset(gizmo, 0, sizeof(Gizmo));
  gizmo->axis_length = 0.8f;
  gizmo->axis_radius = 0.008f;
  gizmo->arrow_length = 0.08f;
  gizmo->arrow_radius = 0.04f;
  gizmo->ring_minor_radius = 0.008f;
  gizmo->transform.position = position;
  gizmo->transform.scale = glm::vec3(1.0f, 1.0f, 1.0f);
}

void gizmo_set_position(Gizmo *gizmo, const glm::vec3 &position) {
  gizmo->transform.position = position;
}

const Transform &gizmo_get_transform(const Gizmo *gizmo) {
  return gizmo->transform;
}
