#include "../headers/box.hpp"

#include <glm/glm.hpp>

#include <stdexcept>

// +
Mesh PFN_vkCreateGraphicsPipelineBox(float width, float height, float depth)
{
  if (width <= 0.0f || height <= 0.0f || depth <= 0.0f) {
    throw std::invalid_argument("invalid box size");
  }

  const glm::vec3 half{ width * 0.5f, height * 0.5f, depth * 0.5f };
  const glm::vec3 center{ 0.0f, half.y, 0.0f };

  struct Face { glm::vec3 n, u, v; };

  // направляющие: n = u x v
  const Face faces[] = {
    { { 1, 0, 0}, { 0, 0,-1}, {0, 1, 0} }, // +X
    { {-1, 0, 0}, { 0, 0, 1}, {0, 1, 0} }, // -X
    { { 0, 1, 0}, { 1, 0, 0}, {0, 0,-1} }, // +Y
    { { 0,-1, 0}, { 1, 0, 0}, {0, 0, 1} }, // -Y
    { { 0, 0, 1}, { 1, 0, 0}, {0, 1, 0} }, // +Z
    { { 0, 0,-1}, {-1, 0, 0}, {0, 1, 0} }, // -Z
  };

  Mesh mesh;

  // 4 own vertices per face so every face gets a flat normal
  // build faces
  for (const Face& f : faces) {
    glm::vec3 c = center + f.n * half;
    glm::vec3 u = f.u * half;
    glm::vec3 v = f.v * half;

    auto first = static_cast<uint32_t>(mesh.vertices.size());

    mesh.vertices.push_back({ c - u - v, f.n, { 0.0f, 1.0f } });
    mesh.vertices.push_back({ c + u - v, f.n, { 1.0f, 1.0f } });
    mesh.vertices.push_back({ c + u + v, f.n, { 1.0f, 0.0f } });
    mesh.vertices.push_back({ c - u + v, f.n, { 0.0f, 0.0f } });

    // indices = { { {0,1,2}, {0,2,3} } <-- first face }, { {2,5,6}, {2,3,4} } <-- second face ...
    mesh.indices.insert(mesh.indices.end(),
        { first, first + 1, first + 2, first, first + 2, first + 3 });
  }

  return mesh;
}
