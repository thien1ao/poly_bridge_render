#pragma once

#include <vulkan/vulkan.h>

#include <cstring>
#include <vector>

struct GLFWwindow;

class Swapchain {
public:
  Swapchain() = default;
  ~Swapchain() { destroy(); }

  Swapchain(const Swapchain&) = delete;
  Swapchain& operator = (const Swapchain&) = delete;

  void create(VkPhysicalDevice physicalDevice,
              VkDevice device,
              VkSurfaceKHR surface,
              GLFWwindow* window,
              uint32_t graphicsFamily,
              uint32_t presentFamily);

  void destroy();

VkSwapchainKHR handle() const { return swapchain_; }
  VkFormat format() const { return format_; }
  VkExtent2D extent() const { return extent_; }
  const std::vector<VkImageView>& imageViews() const { return imageViews_; }

private:
  void createImageViews();

  VkDevice device_ = VK_NULL_HANDLE;
  VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
  std::vector<VkImage> images_;
  std::vector<VkImageView> imageViews_;
  VkFormat format_ = VK_FORMAT_UNDEFINED;
  VkExtent2D extent_{};
};

