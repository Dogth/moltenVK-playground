#include "init.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <array>
#include <cstring>
#include <set>
#include <vector>

#include "buffer.hpp"
#include "descriptor.hpp"
#include "fresnel_push_constants.hpp"
#include "mesh.hpp"
#include "pipeline.hpp"
#include "render_pass.hpp"
#include "ubo.hpp"
#include "vk_types.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifndef SHADER_DIR
#define SHADER_DIR "shader/bin"
#endif

bool VulkanApp::initialize() {
  if (!initWindow())
    return false;
  start_time_ = glfwGetTime();

  if (!createInstance())
    return false;
  if (!createSurface())
    return false;
  if (!pickPhysicalDevice())
    return false;
  if (!createLogicalDevice())
    return false;
  if (!createSwapchain())
    return false;
  if (!createImageViews())
    return false;
  if (!createRenderPass())
    return false;
  if (!createDescriptorSetLayout())
    return false;
  if (!createGraphicsPipeline())
    return false;
  if (!createDepthResources())
    return false;
  if (!createFramebuffers())
    return false;
  if (!createCommandPool())
    return false;
  if (!createVertexBuffer())
    return false;
  if (!createIndexBuffer())
    return false;
  if (!createUniformBuffers())
    return false;
  if (!createDescriptorPool())
    return false;
  if (!createDescriptorSets())
    return false;
  if (!createCommandBuffers())
    return false;
  if (!createSyncObjects())
    return false;

  return true;
}

bool VulkanApp::initWindow() {
  glfwInitVulkanLoader(vkGetInstanceProcAddr);
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  window_ = glfwCreateWindow(800, 600, "Vulkan App", nullptr, nullptr);
  if (!window_) {
    err_ = "Failed to create window";
    return false;
  }
  return true;
}

bool VulkanApp::createInstance() {
  VkApplicationInfo app_info{};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "vulkan_moltenvk_demo";
  app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
  app_info.pEngineName = "none";
  app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
  app_info.apiVersion = VK_API_VERSION_1_2;

  uint32_t glfw_ext_count = 0;
  const char **glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
  if (!glfw_exts) {
    err_ = "GLFW: Vulkan surface extensions not available";
    return false;
  }
  std::vector<const char *> extensions(glfw_exts, glfw_exts + glfw_ext_count);

  VkInstanceCreateFlags create_flags = 0;
#if defined(__APPLE__) || defined(VK_APP_USE_PORTABILITY)
  extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
  create_flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

  VkInstanceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;
  create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  create_info.ppEnabledExtensionNames = extensions.data();
  create_info.flags = create_flags;

  if (vkCreateInstance(&create_info, nullptr, &instance_) != VK_SUCCESS) {
    err_ = "vkCreateInstance failed";
    return false;
  }
  return true;
}

bool VulkanApp::createSurface() {
  if (glfwCreateWindowSurface(instance_, window_, nullptr, &surface_) !=
      VK_SUCCESS) {
    err_ = "Failed to create window surface";
    return false;
  }
  return true;
}

bool VulkanApp::pickPhysicalDevice() {
  uint32_t dev_count = 0;
  vkEnumeratePhysicalDevices(instance_, &dev_count, nullptr);
  if (dev_count == 0) {
    err_ = "no Vulkan-capable devices found";
    return false;
  }

  std::vector<VkPhysicalDevice> devices(dev_count);
  vkEnumeratePhysicalDevices(instance_, &dev_count, devices.data());

  for (auto &candidate : devices) {
    QueueFamilyIndices indices = findQueueFamilies(candidate, surface_);
    if (!indices.isComplete())
      continue;

    device_handle_ = candidate;
    graphics_family_ = indices.graphicsFamily.value();
    present_family_ = indices.presentFamily.value();
    break;
  }

  if (device_handle_ == VK_NULL_HANDLE) {
    err_ = "no GPU with graphics+present support found";
    return false;
  }

  VkPhysicalDeviceProperties props{};
  vkGetPhysicalDeviceProperties(device_handle_, &props);
  device_name_ = props.deviceName;

  depth_format_ = findDepthFormat(device_handle_);
  return true;
}

bool VulkanApp::createLogicalDevice() {
  std::set<uint32_t> unique_families = {graphics_family_, present_family_};

  float queue_priority = 1.0F;
  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  for (uint32_t family : unique_families) {
    VkDeviceQueueCreateInfo qci{};
    qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qci.queueFamilyIndex = family;
    qci.queueCount = 1;
    qci.pQueuePriorities = &queue_priority;
    queue_create_infos.push_back(qci);
  }

  std::vector<const char *> device_extensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};
#if defined(__APPLE__) || defined(VK_APP_USE_PORTABILITY)
  device_extensions.push_back("VK_KHR_portability_subset");
#endif

  VkPhysicalDeviceFeatures features{};

  VkDeviceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.queueCreateInfoCount =
      static_cast<uint32_t>(queue_create_infos.size());
  create_info.pQueueCreateInfos = queue_create_infos.data();
  create_info.pEnabledFeatures = &features;
  create_info.enabledExtensionCount =
      static_cast<uint32_t>(device_extensions.size());
  create_info.ppEnabledExtensionNames = device_extensions.data();

  if (vkCreateDevice(device_handle_, &create_info, nullptr, &device_) !=
      VK_SUCCESS) {
    err_ = "Failed to create logical device";
    return false;
  }

  vkGetDeviceQueue(device_, graphics_family_, 0, &graphics_queue_);
  vkGetDeviceQueue(device_, present_family_, 0, &present_queue_);
  return true;
}

bool VulkanApp::createSwapchain() {
  SwapchainSupportDetails support =
      querySwapchainSupport(device_handle_, surface_);
  VkSurfaceFormatKHR surface_format = chooseSurfaceFormat(support.formats);
  VkPresentModeKHR present_mode = choosePresentMode(support.presentModes);
  VkExtent2D extent = chooseExtent(support.capabilities, window_);

  uint32_t image_count = support.capabilities.minImageCount + 1;
  if (support.capabilities.maxImageCount > 0 &&
      image_count > support.capabilities.maxImageCount) {
    image_count = support.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.surface = surface_;
  create_info.minImageCount = image_count;
  create_info.imageFormat = surface_format.format;
  create_info.imageColorSpace = surface_format.colorSpace;
  create_info.imageExtent = extent;
  create_info.imageArrayLayers = 1;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  uint32_t queue_indices[] = {graphics_family_, present_family_};
  if (graphics_family_ != present_family_) {
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = 2;
    create_info.pQueueFamilyIndices = queue_indices;
  } else {
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  create_info.preTransform = support.capabilities.currentTransform;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_info.presentMode = present_mode;
  create_info.clipped = VK_TRUE;

  if (vkCreateSwapchainKHR(device_, &create_info, nullptr, &swapchain_) !=
      VK_SUCCESS) {
    err_ = "Failed to create swapchain";
    return false;
  }

  vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, nullptr);
  swapchain_images_.resize(image_count);
  vkGetSwapchainImagesKHR(device_, swapchain_, &image_count,
                          swapchain_images_.data());

  swapchain_format_ = surface_format.format;
  swapchain_extent_ = extent;
  return true;
}

bool VulkanApp::createImageViews() {
  swapchain_image_views_.resize(swapchain_images_.size());
  for (size_t i = 0; i < swapchain_images_.size(); ++i) {
    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = swapchain_images_[i];
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = swapchain_format_;
    view_info.components = {
        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device_, &view_info, nullptr,
                          &swapchain_image_views_[i]) != VK_SUCCESS) {
      err_ = "Failed to create image view";
      return false;
    }
  }
  return true;
}

bool VulkanApp::createRenderPass() {
  try {
    render_pass_ = buildRenderPass(device_, swapchain_format_, depth_format_);
  } catch (const std::exception &e) {
    err_ = e.what();
    return false;
  }
  return true;
}

bool VulkanApp::createDescriptorSetLayout() {
  try {
    descriptor_set_layout_ = buildDescriptorSetLayout(device_);
  } catch (const std::exception &e) {
    err_ = e.what();
    return false;
  }
  return true;
}

bool VulkanApp::createGraphicsPipeline() {
  try {
    GraphicsPipelineBundle bundle =
        buildGraphicsPipeline(device_, render_pass_, swapchain_extent_,
                              descriptor_set_layout_, SHADER_DIR);
    graphics_pipeline_ = bundle.pipeline;
    pipeline_layout_ = bundle.layout;
  } catch (const std::exception &e) {
    err_ = e.what();
    return false;
  }
  return true;
}

bool VulkanApp::createDepthResources() {
  VkImageCreateInfo image_info{};
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.extent = {swapchain_extent_.width, swapchain_extent_.height, 1};
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.format = depth_format_;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage(device_, &image_info, nullptr, &depth_image_) !=
      VK_SUCCESS) {
    err_ = "Failed to create depth image";
    return false;
  }

  VkMemoryRequirements mem_reqs{};
  vkGetImageMemoryRequirements(device_, depth_image_, &mem_reqs);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_reqs.size;
  alloc_info.memoryTypeIndex =
      findMemoryType(device_handle_, mem_reqs.memoryTypeBits,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (vkAllocateMemory(device_, &alloc_info, nullptr, &depth_image_memory_) !=
      VK_SUCCESS) {
    err_ = "Failed to allocate depth image memory";
    return false;
  }
  vkBindImageMemory(device_, depth_image_, depth_image_memory_, 0);

  VkImageViewCreateInfo view_info{};
  view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.image = depth_image_;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = depth_format_;
  view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  view_info.subresourceRange.baseMipLevel = 0;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.baseArrayLayer = 0;
  view_info.subresourceRange.layerCount = 1;

  if (vkCreateImageView(device_, &view_info, nullptr, &depth_image_view_) !=
      VK_SUCCESS) {
    err_ = "Failed to create depth image view";
    return false;
  }
  return true;
}

bool VulkanApp::createFramebuffers() {
  framebuffers_.resize(swapchain_image_views_.size());
  for (size_t i = 0; i < swapchain_image_views_.size(); ++i) {
    std::array<VkImageView, 2> attachments = {swapchain_image_views_[i],
                                              depth_image_view_};

    VkFramebufferCreateInfo fb_info{};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.renderPass = render_pass_;
    fb_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    fb_info.pAttachments = attachments.data();
    fb_info.width = swapchain_extent_.width;
    fb_info.height = swapchain_extent_.height;
    fb_info.layers = 1;

    if (vkCreateFramebuffer(device_, &fb_info, nullptr, &framebuffers_[i]) !=
        VK_SUCCESS) {
      err_ = "Failed to create framebuffer";
      return false;
    }
  }
  return true;
}

bool VulkanApp::createCommandPool() {
  VkCommandPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool_info.queueFamilyIndex = graphics_family_;

  if (vkCreateCommandPool(device_, &pool_info, nullptr, &command_pool_) !=
      VK_SUCCESS) {
    err_ = "Failed to create command pool";
    return false;
  }
  return true;
}

bool VulkanApp::createVertexBuffer() {
  VkDeviceSize size =
      sizeof(prism_mesh::kVertices[0]) * prism_mesh::kVertices.size();

  VkBuffer staging_buffer = VK_NULL_HANDLE;
  VkDeviceMemory staging_memory = VK_NULL_HANDLE;
  try {
    createBuffer(device_handle_, device_, size,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 staging_buffer, staging_memory);

    void *data = nullptr;
    vkMapMemory(device_, staging_memory, 0, size, 0, &data);
    memcpy(data, prism_mesh::kVertices.data(), static_cast<size_t>(size));
    vkUnmapMemory(device_, staging_memory);

    createBuffer(device_handle_, device_, size,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffer_,
                 vertex_buffer_memory_);

    copyBuffer(device_, command_pool_, graphics_queue_, staging_buffer,
               vertex_buffer_, size);
  } catch (const std::exception &e) {
    err_ = e.what();
    if (staging_buffer != VK_NULL_HANDLE)
      vkDestroyBuffer(device_, staging_buffer, nullptr);
    if (staging_memory != VK_NULL_HANDLE)
      vkFreeMemory(device_, staging_memory, nullptr);
    return false;
  }

  vkDestroyBuffer(device_, staging_buffer, nullptr);
  vkFreeMemory(device_, staging_memory, nullptr);
  return true;
}

bool VulkanApp::createIndexBuffer() {
  VkDeviceSize size =
      sizeof(prism_mesh::kIndices[0]) * prism_mesh::kIndices.size();

  VkBuffer staging_buffer = VK_NULL_HANDLE;
  VkDeviceMemory staging_memory = VK_NULL_HANDLE;
  try {
    createBuffer(device_handle_, device_, size,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 staging_buffer, staging_memory);

    void *data = nullptr;
    vkMapMemory(device_, staging_memory, 0, size, 0, &data);
    memcpy(data, prism_mesh::kIndices.data(), static_cast<size_t>(size));
    vkUnmapMemory(device_, staging_memory);

    createBuffer(device_handle_, device_, size,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                     VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index_buffer_,
                 index_buffer_memory_);

    copyBuffer(device_, command_pool_, graphics_queue_, staging_buffer,
               index_buffer_, size);
  } catch (const std::exception &e) {
    err_ = e.what();
    if (staging_buffer != VK_NULL_HANDLE)
      vkDestroyBuffer(device_, staging_buffer, nullptr);
    if (staging_memory != VK_NULL_HANDLE)
      vkFreeMemory(device_, staging_memory, nullptr);
    return false;
  }

  vkDestroyBuffer(device_, staging_buffer, nullptr);
  vkFreeMemory(device_, staging_memory, nullptr);
  return true;
}

bool VulkanApp::createUniformBuffers() {
  VkDeviceSize size = sizeof(UniformBufferObject);
  uniform_buffers_.resize(kMaxFramesInFlight);
  uniform_buffers_memory_.resize(kMaxFramesInFlight);
  uniform_buffers_mapped_.resize(kMaxFramesInFlight);

  try {
    for (int i = 0; i < kMaxFramesInFlight; ++i) {
      createBuffer(device_handle_, device_, size,
                   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                   uniform_buffers_[i], uniform_buffers_memory_[i]);
      vkMapMemory(device_, uniform_buffers_memory_[i], 0, size, 0,
                  &uniform_buffers_mapped_[i]);
    }
  } catch (const std::exception &e) {
    err_ = e.what();
    return false;
  }
  return true;
}

bool VulkanApp::createDescriptorPool() {
  try {
    descriptor_pool_ = buildDescriptorPool(device_, kMaxFramesInFlight);
  } catch (const std::exception &e) {
    err_ = e.what();
    return false;
  }
  return true;
}

bool VulkanApp::createDescriptorSets() {
  try {
    descriptor_sets_ = buildDescriptorSets(device_, descriptor_set_layout_,
                                           descriptor_pool_, uniform_buffers_);
  } catch (const std::exception &e) {
    err_ = e.what();
    return false;
  }
  return true;
}

bool VulkanApp::createCommandBuffers() {
  command_buffers_.resize(kMaxFramesInFlight);

  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = command_pool_;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount =
      static_cast<uint32_t>(command_buffers_.size());

  if (vkAllocateCommandBuffers(device_, &alloc_info, command_buffers_.data()) !=
      VK_SUCCESS) {
    err_ = "Failed to allocate command buffers";
    return false;
  }
  return true;
}

bool VulkanApp::createSyncObjects() {
  image_available_semaphores_.resize(kMaxFramesInFlight);
  render_finished_semaphores_.resize(kMaxFramesInFlight);
  in_flight_fences_.resize(kMaxFramesInFlight);

  VkSemaphoreCreateInfo sem_info{};
  sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fence_info{};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (int i = 0; i < kMaxFramesInFlight; ++i) {
    if (vkCreateSemaphore(device_, &sem_info, nullptr,
                          &image_available_semaphores_[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device_, &sem_info, nullptr,
                          &render_finished_semaphores_[i]) != VK_SUCCESS ||
        vkCreateFence(device_, &fence_info, nullptr, &in_flight_fences_[i]) !=
            VK_SUCCESS) {
      err_ = "Failed to create synchronization objects";
      return false;
    }
  }
  return true;
}

void VulkanApp::updateUniformBuffer(uint32_t current_image) {
  auto time = static_cast<float>(glfwGetTime() - start_time_);

  UniformBufferObject ubo{};

  ubo.model = glm::rotate(glm::mat4(1.0F), time * glm::radians(45.0F),
                          glm::vec3(0.25F, 0.5F, 0.75F));
  ubo.view = glm::lookAt(glm::vec3(1.0F, 1.0F, 1.0F), glm::vec3(0.0F),
                         glm::vec3(0.0F, 1.0F, 0.0F));
  ubo.proj = glm::perspective(glm::radians(90.0F),
                              static_cast<float>(swapchain_extent_.width) /
                                  static_cast<float>(swapchain_extent_.height),
                              0.01F, 10.0F);

  ubo.proj[1][1] *= -1.0F;

  memcpy(uniform_buffers_mapped_[current_image], &ubo, sizeof(ubo));
}

void VulkanApp::recordCommandBuffer(VkCommandBuffer cmd, uint32_t image_index) {
  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  vkBeginCommandBuffer(cmd, &begin_info);

  std::array<VkClearValue, 2> clear_values{};
  clear_values[0].color = {{0.02F, 0.02F, 0.05F, 1.0F}};
  clear_values[1].depthStencil = {1.0F, 0};

  VkRenderPassBeginInfo rp_begin{};
  rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  rp_begin.renderPass = render_pass_;
  rp_begin.framebuffer = framebuffers_[image_index];
  rp_begin.renderArea.offset = {0, 0};
  rp_begin.renderArea.extent = swapchain_extent_;
  rp_begin.clearValueCount = static_cast<uint32_t>(clear_values.size());
  rp_begin.pClearValues = clear_values.data();

  vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_);

  VkBuffer vertex_buffers[] = {vertex_buffer_};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(cmd, 0, 1, vertex_buffers, offsets);
  vkCmdBindIndexBuffer(cmd, index_buffer_, 0, VK_INDEX_TYPE_UINT16);

  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline_layout_, 0, 1,
                          &descriptor_sets_[current_frame_], 0, nullptr);

  FresnelPushConstants pc{};
  pc.cameraPosWS[0] = 1.0F;
  pc.cameraPosWS[1] = 1.0F;
  pc.cameraPosWS[2] = 3.0F;
  pc.f0Scalar = 0.09F;
  pc.lightDirWS[0] = 1.0F;
  pc.lightDirWS[1] = 1.0F;
  pc.lightDirWS[2] = 1.0F;
  pc.albedo[0] = 0.75F;
  pc.albedo[1] = 0.5F;
  pc.albedo[2] = 0.25F;

  vkCmdPushConstants(cmd, pipeline_layout_, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                     sizeof(pc), &pc);

  vkCmdDrawIndexed(cmd, static_cast<uint32_t>(prism_mesh::kIndices.size()), 1,
                   0, 0, 0);

  vkCmdEndRenderPass(cmd);
  vkEndCommandBuffer(cmd);
}

void VulkanApp::run() { mainLoop(); }

void VulkanApp::mainLoop() {
  while (!glfwWindowShouldClose(window_)) {
    glfwPollEvents();
    drawFrame();
  }
  vkDeviceWaitIdle(device_);
}

void VulkanApp::drawFrame() {
  vkWaitForFences(device_, 1, &in_flight_fences_[current_frame_], VK_TRUE,
                  UINT64_MAX);

  uint32_t image_index = 0;
  VkResult acquire_result =
      vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX,
                            image_available_semaphores_[current_frame_],
                            VK_NULL_HANDLE, &image_index);
  if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR) {
    // NOTE(dogth): window is not resizeable rn
    return;
  }

  updateUniformBuffer(current_frame_);

  vkResetFences(device_, 1, &in_flight_fences_[current_frame_]);
  vkResetCommandBuffer(command_buffers_[current_frame_], 0);
  recordCommandBuffer(command_buffers_[current_frame_], image_index);

  VkSemaphore wait_semaphores[] = {image_available_semaphores_[current_frame_]};
  VkPipelineStageFlags wait_stages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSemaphore signal_semaphores[] = {
      render_finished_semaphores_[current_frame_]};

  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = wait_semaphores;
  submit_info.pWaitDstStageMask = wait_stages;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffers_[current_frame_];
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = signal_semaphores;

  if (vkQueueSubmit(graphics_queue_, 1, &submit_info,
                    in_flight_fences_[current_frame_]) != VK_SUCCESS) {
    err_ = "Failed to submit draw command buffer";
    return;
  }

  VkSwapchainKHR swapchains[] = {swapchain_};
  VkPresentInfoKHR present_info{};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = signal_semaphores;
  present_info.swapchainCount = 1;
  present_info.pSwapchains = swapchains;
  present_info.pImageIndices = &image_index;

  vkQueuePresentKHR(present_queue_, &present_info);

  current_frame_ = (current_frame_ + 1) % kMaxFramesInFlight;
}

void VulkanApp::cleanup() {
  if (device_) {
    for (auto *fence : in_flight_fences_)
      vkDestroyFence(device_, fence, nullptr);
    for (auto *sem : render_finished_semaphores_)
      vkDestroySemaphore(device_, sem, nullptr);
    for (auto *sem : image_available_semaphores_)
      vkDestroySemaphore(device_, sem, nullptr);
    in_flight_fences_.clear();
    render_finished_semaphores_.clear();
    image_available_semaphores_.clear();

    if (command_pool_)
      vkDestroyCommandPool(device_, command_pool_, nullptr);
    command_pool_ = VK_NULL_HANDLE;

    for (size_t i = 0; i < uniform_buffers_.size(); ++i) {
      vkDestroyBuffer(device_, uniform_buffers_[i], nullptr);
      vkFreeMemory(device_, uniform_buffers_memory_[i], nullptr);
    }
    uniform_buffers_.clear();
    uniform_buffers_memory_.clear();
    uniform_buffers_mapped_.clear();

    if (descriptor_pool_)
      vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);
    descriptor_pool_ = VK_NULL_HANDLE;
    if (descriptor_set_layout_)
      vkDestroyDescriptorSetLayout(device_, descriptor_set_layout_, nullptr);
    descriptor_set_layout_ = VK_NULL_HANDLE;

    if (index_buffer_)
      vkDestroyBuffer(device_, index_buffer_, nullptr);
    if (index_buffer_memory_)
      vkFreeMemory(device_, index_buffer_memory_, nullptr);
    index_buffer_ = VK_NULL_HANDLE;
    index_buffer_memory_ = VK_NULL_HANDLE;

    if (vertex_buffer_)
      vkDestroyBuffer(device_, vertex_buffer_, nullptr);
    if (vertex_buffer_memory_)
      vkFreeMemory(device_, vertex_buffer_memory_, nullptr);
    vertex_buffer_ = VK_NULL_HANDLE;
    vertex_buffer_memory_ = VK_NULL_HANDLE;

    for (auto *fb : framebuffers_)
      vkDestroyFramebuffer(device_, fb, nullptr);
    framebuffers_.clear();

    if (graphics_pipeline_)
      vkDestroyPipeline(device_, graphics_pipeline_, nullptr);
    graphics_pipeline_ = VK_NULL_HANDLE;
    if (pipeline_layout_)
      vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
    pipeline_layout_ = VK_NULL_HANDLE;
    if (render_pass_)
      vkDestroyRenderPass(device_, render_pass_, nullptr);
    render_pass_ = VK_NULL_HANDLE;

    if (depth_image_view_)
      vkDestroyImageView(device_, depth_image_view_, nullptr);
    if (depth_image_)
      vkDestroyImage(device_, depth_image_, nullptr);
    if (depth_image_memory_)
      vkFreeMemory(device_, depth_image_memory_, nullptr);
    depth_image_view_ = VK_NULL_HANDLE;
    depth_image_ = VK_NULL_HANDLE;
    depth_image_memory_ = VK_NULL_HANDLE;

    for (auto *view : swapchain_image_views_)
      vkDestroyImageView(device_, view, nullptr);
    swapchain_image_views_.clear();

    if (swapchain_)
      vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    swapchain_ = VK_NULL_HANDLE;

    vkDestroyDevice(device_, nullptr);
    device_ = VK_NULL_HANDLE;
  }

  if (surface_) {
    vkDestroySurfaceKHR(instance_, surface_, nullptr);
    surface_ = VK_NULL_HANDLE;
  }
  if (instance_) {
    vkDestroyInstance(instance_, nullptr);
    instance_ = VK_NULL_HANDLE;
  }
  if (window_) {
    glfwDestroyWindow(window_);
    window_ = nullptr;
    glfwTerminate();
  }
  device_handle_ = VK_NULL_HANDLE;
}

VulkanApp::~VulkanApp() { cleanup(); }
