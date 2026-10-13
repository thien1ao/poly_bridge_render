#pragma once

#include <vulkan/vulkan.h>

#include <stdexcept>
#include <string>

inline void checkVk(VkResult result, const char* operation)
{
  if (result != VK_SUCCESS) {
    throw std::runtime_error(
        std::string(operation) + " failed, VkResult = " +
        std::to_string(static_cast<int>(result)));
  }
}
