#pragma once

#include <glm/glm.hpp>

struct DirLight {
  alignas(16) glm::vec3 direction;
  alignas(16) glm::vec3 ambient;
  // alignas(16) glm::vec3 diffuse;
  // alignas(16) glm::vec3 specular;
};
