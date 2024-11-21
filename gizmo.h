#pragma once

#include "geometry.h"

// gizmo 当前正处于的操作模式
enum GizmoMode {
  GIZMO_MODE_NONE = 0x000,
  GIZMO_MODE_TRANSLATION = 0x001,
  GIZMO_MODE_ROTATION = 0x010,
  GIZMO_MODE_SCALE = 0x100,
};

struct Gizmo {
  Transform transform;
  GizmoMode active_mode;
  char active_axis; // x, y, z or 0
};

void create_gizmo(const glm::vec3 &position, Gizmo *gizmo);
void destroy_gizmo(Gizmo *gizmo);
