#pragma once

#include "geometry.h"

struct RaycastResult {
  bool hit;
  float distance;
  glm::vec3 point;
  glm::vec3 normal;
};

RaycastResult raycast_sphere(const Ray &ray, const Sphere &sphere);
