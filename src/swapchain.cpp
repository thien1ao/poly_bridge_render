#include "../headers/swapchain.hpp"
#include "../headers/device.hpp"
#include "../headers/vk_check.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <limits>

namespace {

//
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

VkCompositeAlphaFlagBitsKHR chooseCompositeAlpha(const VkSurfaceCapabilitiesKHR& caps)
{
  const VkCompositeAlphaFlagBitsKHR preferred[] = {
    VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
    VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
  };
  for (auto a : preferred) {
    if (caps.supportedCompositeAlpha & a) return a;
  }
  return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
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

void Swapchain::create(const Device& device, VkSurfaceKHR surface, GLFWwindow* window)
{
  destroy();

  device_ = device.logical();
  VkPhysicalDevice physicalDevice = device.physical();

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

  uint32_t imageCount = caps.minImageCount + 1;
  if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) {
    imageCount = caps.maxImageCount;
  }

  VkSwapchainCreateInfoKHR info{};
  info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  info.surface          = surface;
  info.minImageCount    = imageCount;
  info.imageFormat      = surfaceFormat.format;
  info.imageColorSpace  = surfaceFormat.colorSpace;
  info.imageExtent      = extent;
  info.imageArrayLayers = 1;
  info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  const QueueFamilyIndices& families = device.queueFamilies();
  uint32_t familyIndices[] = { *families.graphics, *families.present };

  if (families.graphics != families.present) {
    info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
    info.queueFamilyIndexCount = 2;
    info.pQueueFamilyIndices   = familyIndices;
  } else {
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  info.preTransform   = caps.currentTransform;
  info.compositeAlpha = chooseCompositeAlpha(caps);
  info.presentMode    = presentMode;
  info.clipped        = VK_TRUE;
  info.oldSwapchain   = VK_NULL_HANDLE;

  checkVk(vkCreateSwapchainKHR(device_, &info, nullptr, &swapchain_), "vkCreateSwapchainKHR");

  vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, nullptr);
  images_.resize(imageCount);
  vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, images_.data());

  format_ = surfaceFormat.format;
  extent_ = extent;

  createImageViews();
}

void Swapchain::createImageViews()
{
  imageViews_.resize(images_.size(), VK_NULL_HANDLE);

  for (size_t i = 0; i < images_.size(); ++i) {
    VkImageViewCreateInfo info{};
    info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image    = images_[i];
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format   = format_;
    info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    info.subresourceRange.baseMipLevel   = 0;
    info.subresourceRange.levelCount     = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount     = 1;

    checkVk(vkCreateImageView(device_, &info, nullptr, &imageViews_[i]), "vkCreateImageView");
  }
}

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
