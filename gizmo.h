#pragma once

#include "geometry.h"

struct Gizmo {
  glm::vec3 position;
  glm::vec3 scale;
};

void create_gizmo(Gizmo *gizmo);
void destroy_gizmo(Gizmo *gizmo);
