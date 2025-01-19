#include "geometry_system.h"

void geometry_system_create(GeometrySystemState *state) {}

void geometry_system_destroy(GeometrySystemState *state) {}

void geometry_system_increase_geometry_ref(GeometrySystemState *state, Geometry *geometry) {
  if (auto it = state->geometry_ref_count.find(geometry); it == state->geometry_ref_count.end()) {
    state->geometry_ref_count[geometry] = 1;
  } else {
    it->second++;
  }
}

void geometry_system_decrease_geometry_ref(GeometrySystemState *state, VkContext *vk_context, Geometry *geometry) {
  if (auto it = state->geometry_ref_count.find(geometry); it != state->geometry_ref_count.end()) {
    it->second--;
    if (it->second == 0) {
      state->geometry_ref_count.erase(it);
      destroy_geometry(vk_context, geometry);
      delete geometry;
    }
  }
}
