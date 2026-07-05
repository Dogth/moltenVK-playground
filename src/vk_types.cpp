#include "vk_types.hpp"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <stdexcept>

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device,
                                     VkSurfaceKHR surface) {
  QueueFamilyIndices indices;

  uint32_t family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, nullptr);
  std::vector<VkQueueFamilyProperties> families(family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count,
                                           families.data());

  for (uint32_t i = 0; i < family_count; ++i) {
    if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;
    }

    VkBool32 present_support = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
    if (present_support) {
      indices.presentFamily = i;
    }

    if (indices.isComplete()) {
      break;
    }
  }

  return indices;
}

SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device,
                                              VkSurfaceKHR surface) {
  SwapchainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                            &details.capabilities);

  uint32_t format_count = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
  details.formats.resize(format_count);
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count,
                                       details.formats.data());

  uint32_t mode_count = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &mode_count,
                                            nullptr);
  details.presentModes.resize(mode_count);
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &mode_count,
                                            details.presentModes.data());

  return details;
}

VkSurfaceFormatKHR
chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available) {
  for (const auto &format : available) {
    if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return format;
    }
  }
  return available.empty() ? VkSurfaceFormatKHR{} : available[0];
}

VkPresentModeKHR
choosePresentMode(const std::vector<VkPresentModeKHR> &available) {
  for (const auto &mode : available) {
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return mode;
    }
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR &capabilities,
                        GLFWwindow *window) {
  if (capabilities.currentExtent.width != UINT32_MAX) {
    return capabilities.currentExtent;
  }

  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(window, &width, &height);

  VkExtent2D extent = {static_cast<uint32_t>(width),
                       static_cast<uint32_t>(height)};
  extent.width = std::clamp(extent.width, capabilities.minImageExtent.width,
                            capabilities.maxImageExtent.width);
  extent.height = std::clamp(extent.height, capabilities.minImageExtent.height,
                             capabilities.maxImageExtent.height);
  return extent;
}

VkFormat findDepthFormat(VkPhysicalDevice physical_device) {
  static const std::array<VkFormat, 3> kFormats = {VK_FORMAT_D32_SFLOAT,
                                                   VK_FORMAT_D32_SFLOAT_S8_UINT,
                                                   VK_FORMAT_D24_UNORM_S8_UINT};

  for (VkFormat format : kFormats) {
    VkFormatProperties props{};
    vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);
    if (props.optimalTilingFeatures &
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      return format;
    }
  }

  throw std::runtime_error("Failed to find a supported depth-stencil format");
}

uint32_t findMemoryType(VkPhysicalDevice physical_device, uint32_t type_filter,
                        VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties mem_props{};
  vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);

  for (uint32_t idx = 0; idx < mem_props.memoryTypeCount; ++idx) {
    if ((type_filter & (1U << idx)) &&
        (mem_props.memoryTypes[idx].propertyFlags & properties) == properties) {
      return idx;
    }
  }

  throw std::runtime_error("Failed to find a suitable GPU memory type");
}
