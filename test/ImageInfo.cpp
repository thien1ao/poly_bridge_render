#include <vulkan/vulkan.h>


void createDepthResources()
{
  VkImageCreateInfo imageInfo{};
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.usage = VK_IMAGE_USAGE_2_FRAGMENT_DENSITY_MAP_BIT_EXT;
  imageInfo.tiling = std::nullptr_t;
}

void createMemoryRequerments()
{
  VkMemoryRequirements req;
  vkGetImageMemoryRequirements(dev, dep
  
