#pragma once

#include "geometry.h"

enum GizmoOperation {
  GIZMO_OPERATION_NONE = 0,
  GIZMO_OPERATION_TRANSLATE_X = 1 << 0,
  GIZMO_OPERATION_TRANSLATE_Y = 1 << 1,
  GIZMO_OPERATION_TRANSLATE_Z = 1 << 2,
  GIZMO_OPERATION_ROTATE_X = 1 << 3,
  GIZMO_OPERATION_ROTATE_Y = 1 << 4,
  GIZMO_OPERATION_ROTATE_Z = 1 << 5,
  GIZMO_OPERATION_SCALE_X = 1 << 6,
  GIZMO_OPERATION_SCALE_Y = 1 << 7,
  GIZMO_OPERATION_SCALE_Z = 1 << 8,

  GIZMO_OPERATION_TRANSLATE = GIZMO_OPERATION_TRANSLATE_X | GIZMO_OPERATION_TRANSLATE_Y | GIZMO_OPERATION_TRANSLATE_Z,
  GIZMO_OPERATION_ROTATE = GIZMO_OPERATION_ROTATE_X | GIZMO_OPERATION_ROTATE_Y | GIZMO_OPERATION_ROTATE_Z,
  GIZMO_OPERATION_SCALE = GIZMO_OPERATION_SCALE_X | GIZMO_OPERATION_SCALE_Y | GIZMO_OPERATION_SCALE_Z,
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
  Geometry cube_geometry;

  Transform transform;
  GizmoOperation operation;
  Plane intersection_plane;
  Plane intersection_plane_back;
  glm::vec3 intersection_position;
};

void create_gizmo(const glm::vec3 &position, Gizmo *gizmo);
void destroy_gizmo(Gizmo *gizmo, MeshSystemState *mesh_system_state, VkContext *vk_context);
void gizmo_set_position(Gizmo *gizmo, const glm::vec3 &position);
const Transform &gizmo_get_transform(const Gizmo *gizmo);
