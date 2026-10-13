#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <optional>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;

    bool complete() const { return graphics && present; }
};

class Device {
public:
    Device() = default;
    ~Device() { destroy(); }

    Device(const Device&)            = delete;
    Device& operator=(const Device&) = delete;
    // Device(Device&&)                 = delete;
    // Device& operator=(Device&&)      = delete;

    void create(VkInstance instance, VkSurfaceKHR surface);
    void destroy();

    VkPhysicalDevice physical() const { return physical_; }
    VkDevice         logical()  const { return device_; }
    VkQueue          graphicsQueue() const { return graphicsQueue_; }
    VkQueue          presentQueue()  const { return presentQueue_; }
    const QueueFamilyIndices& queueFamilies() const { return families_; }

private:
    void pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
    void createLogicalDevice();

    VkPhysicalDevice  physical_      = VK_NULL_HANDLE;
    VkDevice          device_        = VK_NULL_HANDLE;
    VkQueue           graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue           presentQueue_  = VK_NULL_HANDLE;
    QueueFamilyIndices families_;
};



