#include "../headers/renderer.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <string>
#include <optional>
#include <vector>

namespace {

void checkVk(VkResult result, const char* operation)
{
  if (result != VK_SUCCESS) {
    throw std::runtime_error(
        std::string(operation) +
        " failed, VkResult = " +
        std::to_string(static_cast<int>(result))
        );
  }
}

struct QueueFamily {
  std::optional<uint32_t> graphics;
  std::optional<uint32_t> present;
  bool complete() const { return graphics && present; }
};

// graphics queus are supported in 10 cases meanwhile present queues may not be supported
QueueFamily findQueueFamily(pickPhysicalDevice device, VkSurfaceKHR surface)
{
  QueueFamily ans;

  uint32_t count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
  std::vector<VkQueueFamilyProperties> families(count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

  for (uint32_t i = 0; i < count; ++i) {
    if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      ans.graphics = i;
    }

    VkBool32 presentSupport = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i




}

}

Renderer::~Renderer()
{
  shutdown();
}

void Renderer::init(GLFWwindow* window)
{
  if (!window) {
    throw std::runtime_error("window initialisation problem");
  }

  if (instance_ != VK_NULL_HANDLE) {
    throw std::runtime_error("render already running");
  }

  window_ = window;

  try {
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
  }
  // (...) any exception
  catch (...) {
    shutdown();
    throw;
  }
}

void Renderer::createInstance()
{
  if (!glfwVulkanSupported()) {
    throw std::runtime_error("vulkan is not supported");
  }

  // glfw returns list of strings --> 2 lvls of pointers
  uint32_t extensionCount = 0;
    const char** extensions =
        glfwGetRequiredInstanceExtensions(&extensionCount);

    if (!extensions || extensionCount == 0) {
        throw std::runtime_error(
            "Required Vulkan window extensions are unavailable"
        );
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Sphere Renderer";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "My Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;

    checkVk(vkCreateInstance(&createInfo, nullptr, &instance_), "vkCreateInstance");
}

void Renderer::createSurface()
{
  checkVk(glfwCreateWindowSurface(
        instance_,
        window_,
        nullptr,
        &surface_
      ),
      "glfwCreateWindowSurface"
  );
}

void Renderer::shutdown()
{
    if (surface_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }

    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }

    window_ = nullptr;
}

void Renderer::pickPhysicalDevice()
{


}

void Renderer::createLogicalDevice()
{

}

void Renderer::drawFrame()
{

}


