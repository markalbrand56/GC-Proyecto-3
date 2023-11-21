#pragma once

#include <glm/glm.hpp>

struct Intersect {
  bool isIntersecting = false;
  float dist = 0.0f;
  glm::vec3 point;
  glm::vec3 normal;
  float u = 0.0f;
  float v = 0.0f;
};

