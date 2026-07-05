#pragma once

#include "vk_types.hpp"
#include <stdexcept>
#include <vulkan/vulkan.h>

inline void createBuffer(VkPhysicalDevice physical_device, VkDevice device,
                         VkDeviceSize size, VkBufferUsageFlags usage,
                         VkMemoryPropertyFlags properties, VkBuffer &out_buffer,
                         VkDeviceMemory &out_memory) {
  VkBufferCreateInfo buffer_info{};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = size;
  buffer_info.usage = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &buffer_info, nullptr, &out_buffer) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create buffer");
  }

  VkMemoryRequirements mem_reqs{};
  vkGetBufferMemoryRequirements(device, out_buffer, &mem_reqs);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_reqs.size;
  alloc_info.memoryTypeIndex =
      findMemoryType(physical_device, mem_reqs.memoryTypeBits, properties);

  if (vkAllocateMemory(device, &alloc_info, nullptr, &out_memory) !=
      VK_SUCCESS) {
    vkDestroyBuffer(device, out_buffer, nullptr);
    throw std::runtime_error("Failed to allocate buffer memory");
  }

  vkBindBufferMemory(device, out_buffer, out_memory, 0);
};

inline void copyBuffer(VkDevice device, VkCommandPool command_pool,
                       VkQueue queue, VkBuffer src, VkBuffer dst,
                       VkDeviceSize size) {
  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandPool = command_pool;
  alloc_info.commandBufferCount = 1;

  VkCommandBuffer cmd;
  vkAllocateCommandBuffers(device, &alloc_info, &cmd);

  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cmd, &begin_info);

  VkBufferCopy copy_region{};
  copy_region.size = size;
  vkCmdCopyBuffer(cmd, src, dst, 1, &copy_region);

  vkEndCommandBuffer(cmd);

  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &cmd;

  vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);

  vkFreeCommandBuffers(device, command_pool, 1, &cmd);
};
