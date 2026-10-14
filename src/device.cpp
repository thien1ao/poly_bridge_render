#include "../headers/device.hpp"
#include "../headers/vk_check.hpp"

#include <cstring>
#include <set>
#include <stdexcept>
#include <vector>

namespace {

const char* const kDeviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

// +
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    QueueFamilyIndices ans;

    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

    for (uint32_t i = 0; i < count; ++i) {
        bool graphics = families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        // one family doing both is the fastest setup, take it right away
        if (graphics && presentSupport) {
            ans.graphics = i;
            ans.present  = i;
            return ans;
        }

        if (graphics && !ans.graphics)       ans.graphics = i;
        if (presentSupport && !ans.present)  ans.present  = i;
    }
    return ans;
}

// +
bool supportsExtensions(VkPhysicalDevice device)
{
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> available(count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, available.data());

    for (const auto& e : available)
      if (std::strcmp(e.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
            return true;
    return false;
}

// +
bool swapchainAdequate(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    uint32_t formats = 0, modes = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formats, nullptr);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &modes, nullptr);
    return formats > 0 && modes > 0;
}

// can rate device only if extenstions supported
// queue Families are found
// swapchain supports formats and modes
//
// each type of GPU gets number of points
// support of iGPU
int rateDevice(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    if (!findQueueFamilies(device, surface).complete()) return 0;
    if (!supportsExtensions(device))                    return 0;
    if (!swapchainAdequate(device, surface))            return 0;

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(device, &props);

    int score = 0;
    switch (props.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   score += 1000; break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: score += 500;  break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    score += 100;  break;
        default: break;
    }
    score += static_cast<int>(props.limits.maxImageDimension2D / 1024);
    return score;
}

}

// +
void Device::create(VkInstance instance, VkSurfaceKHR surface)
{
    pickPhysicalDevice(instance, surface);
    createLogicalDevice();
}

// to pick physicalDevice enumerate available
// after devices in the list score them
// after device scores find queue Famliies
//
// +
void Device::pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
{
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (count == 0)
        throw std::runtime_error("failed to find GPUs with Vulkan support");

    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance, &count, devices.data());

    int bestScore = 0;
    for (VkPhysicalDevice d : devices) {
        int score = rateDevice(d, surface);
        if (score > bestScore) {
            bestScore = score;
            physical_ = d;
        }
    }

    if (physical_ == VK_NULL_HANDLE)
       throw std::runtime_error("failed to find a suitable GPU");

    families_ = findQueueFamilies(physical_, surface);
}

// logical device = interface for physical device
// queueFamilies if present and graphics are the same: remove duplocates
//
// for queue family create info (queue interface)
// NAME
// PRIORITY
// TYPE
//
// connect to features struct
// mark device FEATURES
//
// +
void Device::createLogicalDevice()
{
    std::set<uint32_t> uniqueFamilies = { *families_.graphics, *families_.present };

    float priority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    for (uint32_t family : uniqueFamilies) {
        VkDeviceQueueCreateInfo info{};
        info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        info.queueFamilyIndex = family;
        info.queueCount       = 1;
        info.pQueuePriorities = &priority;
        queueInfos.push_back(info);
    }

    VkPhysicalDeviceFeatures features{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueInfos.size());
    createInfo.pQueueCreateInfos       = queueInfos.data();
    createInfo.pEnabledFeatures        = &features;
    createInfo.enabledExtensionCount   = 1;
    createInfo.ppEnabledExtensionNames = kDeviceExtensions;

    checkVk(vkCreateDevice(physical_, &createInfo, nullptr, &device_), "vkCreateDevice");

    vkGetDeviceQueue(device_, *families_.graphics, 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, *families_.present,  0, &presentQueue_);
}

// destroy physical device
// free queus
// free families
void Device::destroy()
{
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }

    physical_      = VK_NULL_HANDLE;
    graphicsQueue_ = VK_NULL_HANDLE;
    presentQueue_  = VK_NULL_HANDLE;
    families_      = {};
}
