#pragma once

#include "geometry.h"

enum GizmoAction {
  GIZMO_ACTION_NONE = 0,
  GIZMO_ACTION_TRANSLATE,
  GIZMO_ACTION_ROTATE,
  GIZMO_ACTION_SCALE,
};

enum GizmoAxis {
  GIZMO_AXIS_NONE = 0,
  GIZMO_AXIS_X = 1 << 0,
  GIZMO_AXIS_Y = 1 << 1,
  GIZMO_AXIS_Z = 1 << 2,
  GIZMO_AXIS_XYZ = GIZMO_AXIS_X | GIZMO_AXIS_Y | GIZMO_AXIS_Z,
};

struct GizmoConfig {
  float axis_length;
  float axis_radius;
  float arrow_length;
  float arrow_radius;
  float ring_major_radius;
  float ring_minor_radius;
};

struct Gizmo {
  GizmoConfig config;

  Geometry axis_geometry; // the body of the axes
  Geometry arrow_geometry; // the head of the axes
  Geometry ring_geometry;
  Geometry sector_geometry;
  Geometry cube_geometry;

  Transform transform;
  GizmoAction action;
  GizmoAxis active_axis;
  Plane intersection_plane;
  Plane intersection_plane_back;
  glm::vec3 intersection_position;
};

void create_gizmo(MeshSystemState *mesh_system_state, VkContext *vk_context, const glm::vec3 &position, Gizmo *gizmo);
void destroy_gizmo(Gizmo *gizmo, MeshSystemState *mesh_system_state, VkContext *vk_context);
void gizmo_set_position(Gizmo *gizmo, const glm::vec3 &position);
const Transform &gizmo_get_transform(const Gizmo *gizmo);
