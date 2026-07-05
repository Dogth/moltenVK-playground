#pragma once

#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

struct GLFWwindow;

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete() const {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

struct SwapchainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities{};
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device,
                                     VkSurfaceKHR surface);

SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device,
                                              VkSurfaceKHR surface);

VkSurfaceFormatKHR
chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available);

VkPresentModeKHR
choosePresentMode(const std::vector<VkPresentModeKHR> &available);

VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR &capabilities,
                        GLFWwindow *window);

VkFormat findDepthFormat(VkPhysicalDevice physical_device);

uint32_t findMemoryType(VkPhysicalDevice physical_device, uint32_t type_filter,
                        VkMemoryPropertyFlags properties);
