#include "init.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("VulkanApp initializes an instance and finds a device", "[vulkan]") {
  VulkanApp app;
  REQUIRE(app.initialize());
  REQUIRE_FALSE(app.deviceName().empty());
  app.cleanup();
}

TEST_CASE("VulkanApp cleanup is idempotent", "[vulkan]") {
  VulkanApp app;
  REQUIRE(app.initialize());
  app.cleanup();
  app.cleanup();
  SUCCEED();
}
