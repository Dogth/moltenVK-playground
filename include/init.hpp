#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

struct GLFWwindow;

class VulkanApp {
public:
  bool initialize();
  void run();
  void cleanup();

  const std::string &lastError() const { return err_; }
  const std::string &deviceName() const { return device_name_; }

  VulkanApp() = default;
  ~VulkanApp();
  VulkanApp(const VulkanApp &) = delete;
  VulkanApp &operator=(const VulkanApp &) = delete;

private:
  static constexpr int kMaxFramesInFlight = 2;

  bool initWindow();
  bool createInstance();
  bool createSurface();
  bool pickPhysicalDevice();
  bool createLogicalDevice();
  bool createSwapchain();
  bool createImageViews();
  bool createRenderPass();
  bool createDescriptorSetLayout();
  bool createGraphicsPipeline();
  bool createDepthResources();
  bool createFramebuffers();
  bool createCommandPool();
  bool createVertexBuffer();
  bool createIndexBuffer();
  bool createUniformBuffers();
  bool createDescriptorPool();
  bool createDescriptorSets();
  bool createCommandBuffers();
  bool createSyncObjects();

  void mainLoop();
  void drawFrame();
  void updateUniformBuffer(uint32_t current_image);
  void recordCommandBuffer(VkCommandBuffer cmd, uint32_t image_index);

  GLFWwindow *window_ = nullptr;
  VkInstance instance_ = VK_NULL_HANDLE;
  VkSurfaceKHR surface_ = VK_NULL_HANDLE;
  VkPhysicalDevice device_handle_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;
  std::string device_name_;
  std::string err_;

  uint32_t graphics_family_ = 0;
  uint32_t present_family_ = 0;
  VkQueue graphics_queue_ = VK_NULL_HANDLE;
  VkQueue present_queue_ = VK_NULL_HANDLE;

  VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
  std::vector<VkImage> swapchain_images_;
  std::vector<VkImageView> swapchain_image_views_;
  VkFormat swapchain_format_ = VK_FORMAT_UNDEFINED;
  VkExtent2D swapchain_extent_{};
  std::vector<VkFramebuffer> framebuffers_;

  VkFormat depth_format_ = VK_FORMAT_UNDEFINED;
  VkImage depth_image_ = VK_NULL_HANDLE;
  VkDeviceMemory depth_image_memory_ = VK_NULL_HANDLE;
  VkImageView depth_image_view_ = VK_NULL_HANDLE;

  VkRenderPass render_pass_ = VK_NULL_HANDLE;
  VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
  VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
  VkPipeline graphics_pipeline_ = VK_NULL_HANDLE;

  VkCommandPool command_pool_ = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> command_buffers_;

  VkBuffer vertex_buffer_ = VK_NULL_HANDLE;
  VkDeviceMemory vertex_buffer_memory_ = VK_NULL_HANDLE;
  VkBuffer index_buffer_ = VK_NULL_HANDLE;
  VkDeviceMemory index_buffer_memory_ = VK_NULL_HANDLE;

  std::vector<VkBuffer> uniform_buffers_;
  std::vector<VkDeviceMemory> uniform_buffers_memory_;
  std::vector<void *> uniform_buffers_mapped_;

  VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptor_sets_;

  std::vector<VkSemaphore> image_available_semaphores_;
  std::vector<VkSemaphore> render_finished_semaphores_;
  std::vector<VkFence> in_flight_fences_;
  uint32_t current_frame_ = 0;

  double start_time_ = 0.0;
};
