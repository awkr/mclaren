#pragma once

#include "geometry.h"
#include "geometry_system.h"

enum GizmoMode {
  GIZMO_MODE_NONE = 0,
  GIZMO_MODE_TRANSLATE = 1 << 0,
  GIZMO_MODE_ROTATE = 1 << 1,
  GIZMO_MODE_SCALE = 1 << 2,
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
  float cube_size;
  float cube_offset;
};

struct Gizmo {
  GizmoConfig config;

  Geometry axis_geometry; // the body of the axes
  Geometry arrow_geometry; // the head of the axes
  Geometry ring_geometry;
  Geometry cube_geometry;
  Geometry *sector_geometry;

  Transform transform;
  GizmoMode mode;
  GizmoAxis axis;

  // runtime data
  bool is_active;

  // translation runtime data
  Plane intersection_plane; // in the world space
  glm::vec3 intersection_position; // in the world space

  // rotation runtime data
  glm::vec3 rotation_plane_normal; // in the coordinate space defined by the gizmo
  glm::vec3 rotation_start_pos; // in the coordinate space defined by the gizmo
  char rotation_clock_dir; // '0': not set, 'C': counterclockwise, 'c': clockwise
  bool is_rotation_clock_dir_locked;
};

void create_gizmo(MeshSystemState *mesh_system_state, VkContext *vk_context, const glm::vec3 &position, Gizmo *gizmo);
void destroy_gizmo(Gizmo *gizmo, GeometrySystemState *geometry_system_state, VkContext *vk_context);
void gizmo_set_position(Gizmo *gizmo, const glm::vec3 &position); // world space
void gizmo_set_scale(Gizmo *gizmo, const glm::vec3 &scale); // world space
const Transform &gizmo_get_transform(const Gizmo *gizmo);
