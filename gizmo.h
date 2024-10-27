#pragma once

#include "geometry.h"

struct Gizmo {
  Geometry axis_geometry;
};

void create_gizmo(Gizmo *gizmo);
void destroy_gizmo(Gizmo *gizmo);
