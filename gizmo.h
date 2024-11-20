#pragma once

#include "geometry.h"

enum GizmoMode {
  GIZMO_MODE_NONE = 0x000,
  GIZMO_MODE_TRANSLATION = 0x01,
  GIZMO_MODE_ROTATION = 0x10,
  GIZMO_MODE_SCALE = 0x100,
};

struct Gizmo {
  GizmoMode mode;
  glm::vec3 position;
  glm::vec3 scale;
  char activated_axis; // x, y, z or 0
};

void create_gizmo(GizmoMode mode, Gizmo *gizmo);
void destroy_gizmo(Gizmo *gizmo);
