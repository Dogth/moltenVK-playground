#pragma once

#include <string>
#include <vulkan/vulkan.h>

struct GraphicsPipelineBundle {
  VkPipeline pipeline = VK_NULL_HANDLE;
  VkPipelineLayout layout = VK_NULL_HANDLE;
};

GraphicsPipelineBundle buildGraphicsPipeline(
    VkDevice device, VkRenderPass render_pass, VkExtent2D extent,
    VkDescriptorSetLayout descriptor_set_layout, const std::string &shader_dir);
