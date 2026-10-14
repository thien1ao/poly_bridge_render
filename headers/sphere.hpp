#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv; // texture coordinates 0..1, used for patterns like the checker ball
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

Mesh createSphere(uint32_t sectors = 12, uint32_t stacks = 6);
