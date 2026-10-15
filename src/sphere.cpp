#include "../headers/sphere.hpp"

#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

// sectors: horizontal lines stacks: vertical lines
Mesh createSphere(uint32_t sectors, uint32_t stacks) {
  if (sectors < 3 || stacks < 2) {
    throw std::invalid_argument("invalid sphere resolution");
  }

  constexpr float pi = 3.14159265358979323846f;

  Mesh mesh;

  for (uint32_t i = 0; i <= stacks; ++i) {
    float theta = pi * static_cast<float>(i) / stacks;

    for (uint32_t j = 0; j <= sectors; ++j) {
      float phi = 2.0f * pi * static_cast<float>(j) / sectors;

      glm::vec3 p{std::sin(theta) * std::cos(phi), std::cos(theta),
                  std::sin(theta) * std::sin(phi)};

      // u goes around the sphere, v from top to bottom
      glm::vec2 uv{static_cast<float>(j) / sectors, static_cast<float>(i) / stacks};

      mesh.vertices.push_back({p, p, uv});
    }
  }

  // insert(vector.end() <-- inserts at the end
  const uint32_t rowSize = sectors + 1;

  for (uint32_t i = 0; i < stacks; ++i) {
    for (uint32_t j = 0; j < sectors; ++j) {
      uint32_t a = i * rowSize + j;
      uint32_t b = a + rowSize;

      if (i != 0) {
        mesh.indices.insert(mesh.indices.end(), {a, a + 1, b});
      }

      if (i != stacks - 1) {
        mesh.indices.insert(mesh.indices.end(), {a + 1, b + 1, b});
      }
    }
  }

  return mesh;
}
