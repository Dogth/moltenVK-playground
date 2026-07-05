#include "init.hpp"
#include <iostream>

int main() {
  VulkanApp app;

  if (!app.initialize()) {
    std::cerr << "Failed to initialize Vulkan: " << app.lastError() << "\n";
    return 1;
  }

  app.run();

  app.cleanup();
  return 0;
}
