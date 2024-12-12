#include "raycast.h"

RaycastResult raycast_sphere(const Ray &ray, const Sphere &sphere) {
  RaycastResult raycast_result{};
  const glm::vec3 ray_to_sphere = sphere.center - ray.origin;
  const float length = glm::dot(ray.direction, ray_to_sphere);
  const float distance = glm::length(ray_to_sphere);
  const float d = sphere.radius * sphere.radius - (distance * distance - length * length);
  raycast_result.hit = d >= 0.0f;
  if (distance < sphere.radius) { // 射线起点在球体内部
    raycast_result.distance = length + sqrt(d);
    raycast_result.point = ray.origin + raycast_result.distance * ray.direction;
    raycast_result.normal = -glm::normalize(raycast_result.point - sphere.center);
  } else { // 射线起点在球体外部
    raycast_result.distance = length - sqrt(d);
    raycast_result.point = ray.origin + raycast_result.distance * ray.direction;
    raycast_result.normal = glm::normalize(raycast_result.point - sphere.center);
  }
  return raycast_result;
}
