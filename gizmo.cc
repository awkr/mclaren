#include "gizmo.h"

void create_gizmo(const glm::vec3 &position, Gizmo *gizmo) {
  memset(gizmo, 0, sizeof(Gizmo));
  gizmo->transform.position = position;
  gizmo->transform.scale = glm::vec3(1.0f, 1.0f, 1.0f);
}
