#include "../headers/swapchain.hpp"
#include "../headers/vk_check.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <limits>

// namespace for Swapchain helper functions
namespace {

VkSurfaceFormatKHR chooseSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& formats)
{
  for (const auto& f : formats) {
    if (f.format == VK_FORMAT_B8G8R8A8_SRGB &&
        f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return f;
    }
  }
  return formats[0];
}

VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& modes)
{
  for (auto m : modes) {
    if (m == VK_PRESENT_MODE_MAILBOX_KHR) return m;
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& caps, GLFWwindow* window)
{
  if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return caps.currentExtent;
  }

  int w = 0, h = 0;
  glfwGetFramebufferSize(window, &w, &h);

  VkExtent2D e = { static_cast<uint32_t>(w), static_cast<uint32_t>(h) };
  e.width  = std::clamp(e.width,  caps.minImageExtent.width,  caps.maxImageExtent.width);
  e.height = std::clamp(e.height, caps.minImageExtent.height, caps.maxImageExtent.height);
  return e;
}

}

// ^^^^^^
// ||||||
// bellow available functionality above functions to choose from this functionality
void Swapchain::create(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VkSurfaceKHR surface,
    GLFWwindow *window,
    uint32_t graphicsFamily,
    uint32_t presentFamily)
{
  device_ = device; // hdevice copy to nullhptr

  VkSurfaceCapabilitiesKHR caps;

  checkVk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &caps), "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

  uint32_t formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
  std::vector<VkSurfaceFormatKHR> formats(formatCount);
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

  uint32_t modeCount = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &modeCount, nullptr);
  std::vector<VkPresentModeKHR> modes(modeCount);
  vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &modeCount, modes.data());

  VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(formats);
  VkPresentModeKHR presentMode     = choosePresentMode(modes);
  VkExtent2D extent                = chooseExtent(caps, window);

  // TODO
  // pointer to a struct with swapchain  parameters
  //
}

void Swapchain::createImageViews()
{
  imageViews_.resize(imageViews_.size(), VK_NULL_HANDLE);
  // TODO
}

// images are not accessable --> use ImageView
// containts addittional metadata
 void Swapchain::destroy()
{
  if (device_ == VK_NULL_HANDLE) return;

  for (VkImageView view : imageViews_) {
    if (view != VK_NULL_HANDLE) vkDestroyImageView(device_, view, nullptr);
  }
  imageViews_.clear();
  images_.clear();

  if (swapchain_ != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    swapchain_ = VK_NULL_HANDLE;
  }

  device_ = VK_NULL_HANDLE;
}
