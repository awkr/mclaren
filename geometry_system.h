#pragma once

#include "geometry.h"
#include <unordered_map>

struct GeometrySystemState {
  std::unordered_map<Geometry *, uint64_t> geometry_ref_count;
};

void geometry_system_create(GeometrySystemState *state);
void geometry_system_destroy(GeometrySystemState *state);
void geometry_system_increase_geometry_ref(GeometrySystemState *state, Geometry *geometry);
void geometry_system_decrease_geometry_ref(GeometrySystemState *state, VkContext *vk_context, Geometry *geometry);
