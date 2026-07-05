#pragma once

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

class Shader {
public:
  Shader(VkDevice device, const std::string &filename,
         VkShaderStageFlagBits stage)
      : device_(device), stage_(stage) {
    auto code = readFile(filename);
    module_ = createShaderModule(code);
  }

  ~Shader() {
    if (module_ != VK_NULL_HANDLE) {
      vkDestroyShaderModule(device_, module_, nullptr);
    }
  }

  Shader(const Shader &) = delete;
  Shader &operator=(const Shader &) = delete;

  VkPipelineShaderStageCreateInfo getStageCreateInfo() const {
    VkPipelineShaderStageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.stage = stage_;
    info.module = module_;
    info.pName = "main";
    return info;
  }

private:
  VkDevice device_;
  VkShaderModule module_ = VK_NULL_HANDLE;
  VkShaderStageFlagBits stage_;

  static std::vector<char> readFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
      throw std::runtime_error("Failed to open file: " + filename);
    }

    size_t file_size = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), file_size);
    file.close();

    return buffer;
  }

  VkShaderModule createShaderModule(const std::vector<char> &code) const {
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t *>(code.data());

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device_, &create_info, nullptr, &shader_module) !=
        VK_SUCCESS) {
      throw std::runtime_error("Failed to create shader module!");
    }

    return shader_module;
  }
};
