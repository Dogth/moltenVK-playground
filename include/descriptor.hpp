#pragma once

#include "ubo.hpp"
#include <vector>
#include <vulkan/vulkan.h>

inline VkDescriptorSetLayout buildDescriptorSetLayout(VkDevice device) {
  VkDescriptorSetLayoutBinding ubo_binding{};
  ubo_binding.binding = 0;
  ubo_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  ubo_binding.descriptorCount = 1;
  ubo_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkDescriptorSetLayoutCreateInfo layout_info{};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.bindingCount = 1;
  layout_info.pBindings = &ubo_binding;

  VkDescriptorSetLayout layout;
  if (vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &layout) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create descriptor set layout");
  }
  return layout;
};

inline VkDescriptorPool buildDescriptorPool(VkDevice device,
                                            uint32_t frame_count) {
  VkDescriptorPoolSize pool_size{};
  pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_size.descriptorCount = frame_count;

  VkDescriptorPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.poolSizeCount = 1;
  pool_info.pPoolSizes = &pool_size;
  pool_info.maxSets = frame_count;

  VkDescriptorPool pool;
  if (vkCreateDescriptorPool(device, &pool_info, nullptr, &pool) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create descriptor pool");
  }
  return pool;
};

inline std::vector<VkDescriptorSet>
buildDescriptorSets(VkDevice device, VkDescriptorSetLayout layout,
                    VkDescriptorPool pool,
                    const std::vector<VkBuffer> &uniform_buffers) {
  std::vector<VkDescriptorSetLayout> layouts(uniform_buffers.size(), layout);

  VkDescriptorSetAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.descriptorPool = pool;
  alloc_info.descriptorSetCount = static_cast<uint32_t>(layouts.size());
  alloc_info.pSetLayouts = layouts.data();

  std::vector<VkDescriptorSet> sets(uniform_buffers.size());
  if (vkAllocateDescriptorSets(device, &alloc_info, sets.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate descriptor sets");
  }

  for (size_t i = 0; i < uniform_buffers.size(); ++i) {
    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = uniform_buffers[i];
    buffer_info.offset = 0;
    buffer_info.range = sizeof(UniformBufferObject);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = sets[i];
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.descriptorCount = 1;
    write.pBufferInfo = &buffer_info;

    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
  }

  return sets;
};
