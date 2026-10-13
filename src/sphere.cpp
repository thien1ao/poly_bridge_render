#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include "../headers/sphere.hpp"

#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

// Mesh needs detalisation
Mesh createSpehere(uint32_t sectors, uint32_t stacks) {
  if (sectors < 3 || stacks < 2) {
    throw std::invalid_argument("invalid sphere resolution");
  }

  constexpr float pi = 3.14159265358979323846f;

  Mesh mesh;

  // static_cast<float> 0 --> 0.0
  for (uint32_t i = 0; i <= stacks; ++i) {
    // 1/6 --> pi/6 --> 30 grad
    // 2/6 --> pi/3 --> 60 grad
    // ...
    float theta = pi * static_cast<float>(i) / stacks;

    for (uint32_t j = 0; j <= sectors; ++j) {
      float phi = 2.0f * pi * static_cast<float>(j) / sectors;

      glm::vec3 p{std::sin(theta) * std::cos(phi), std::cos(theta),
                  std::sin(theta) * std::sin(phi)};

      // first p for pos second for normal
      mesh.vertices.push_back({p, p});
    }
  }

  // connect vertives
  //
  const uint32_t rowSize = sectors + 1;

  for (uint32_t i = 0; i <= stacks; ++i) {
    for (uint32_t j = 0; j <= sectors; ++j) {
      // getting idx of the vertex
      uint32_t a = i * rowSize + j;
      uint32_t b = a + rowSize;

      // ignore triangles at poles
      // insert append 3 itegers to the end of indices array
      if (i != 0) {
        mesh.indices.insert(mesh.indices.end(), {a, a + 1, b});
      }

      if (i != stacks - 1) {
        mesh.indices.insert(mesh.indices.end(), {a + 1, b + 1, b});
      }
    }
  }

  return mesh;
};
