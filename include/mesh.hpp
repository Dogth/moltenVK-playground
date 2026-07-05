#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

struct Vertex {
  glm::vec3 pos;
  glm::vec3 normal;

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(Vertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return binding;
  }

  static std::array<VkVertexInputAttributeDescription, 2>
  getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attrs{};
    attrs[0].binding = 0;
    attrs[0].location = 0;
    attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[0].offset = offsetof(Vertex, pos);

    attrs[1].binding = 0;
    attrs[1].location = 1;
    attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[1].offset = offsetof(Vertex, normal);
    return attrs;
  }
};

namespace prism_mesh {

inline const std::vector<Vertex> kVertices = {
    // TOP
    {{0.0F, 0.6F, 0.5F}, {0.0F, 0.0F, -1.0F}},
    {{-0.5F, -0.3F, 0.5F}, {0.0F, 0.0F, -1.0F}},
    {{0.5F, -0.3F, 0.5F}, {0.0F, 0.0F, -1.0F}},

    // BOTTOM
    {{0.0F, 0.6F, -0.5F}, {0.0F, 0.0F, 1.0F}},
    {{0.5F, -0.3F, -0.5F}, {0.0F, 0.0F, 1.0F}},
    {{-0.5F, -0.3F, -0.5F}, {0.0F, 0.0F, 1.0F}},

    // SIDES
    {{0.0F, 0.6F, 0.5F}, {0.866F, 0.5F, 0.0F}},
    {{0.0F, 0.6F, -0.5F}, {0.866F, 0.5F, 0.0F}},
    {{-0.5F, -0.3F, -0.5F}, {0.866F, 0.5F, 0.0F}},
    {{-0.5F, -0.3F, 0.5F}, {0.866F, 0.5F, 0.0F}},

    {{-0.5F, -0.3F, 0.5F}, {0.0F, 1.0F, 0.0F}},
    {{-0.5F, -0.3F, -0.5F}, {0.0F, 1.0F, 0.0F}},
    {{0.5F, -0.3F, -0.5F}, {0.0F, 1.0F, 0.0F}},
    {{0.5F, -0.3F, 0.5F}, {0.0F, 1.0F, 0.0F}},

    {{0.5F, -0.3F, 0.5F}, {-0.866F, 0.5F, 0.0F}},
    {{0.5F, -0.3F, -0.5F}, {-0.866F, 0.5F, 0.0F}},
    {{0.0F, 0.6F, -0.5F}, {-0.866F, 0.5F, 0.0F}},
    {{0.0F, 0.6F, 0.5F}, {-0.866F, 0.5F, 0.0F}},
};

inline const std::vector<uint16_t> kIndices = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  6,  8,  9,
    10, 11, 12, 10, 12, 13, 14, 15, 16, 14, 16, 17,
};

} // namespace prism_mesh
