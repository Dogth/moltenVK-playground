#pragma once

#include <cstring>
#include <vulkan/vulkan.h>

struct FresnelPushConstants {
  float cameraPosWS[3];
  float f0Scalar;
  float lightDirWS[3];
  float pad0;
  float albedo[3];
  float pad1;
};

static_assert(
    sizeof(FresnelPushConstants) == 48,
    "FresnelPushConstants size must match the GLSL push_constant block");

inline VkPushConstantRange makeFresnelPushConstantRange() {
  VkPushConstantRange range{};
  range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  range.offset = 0;
  range.size = sizeof(FresnelPushConstants);
  return range;
}
