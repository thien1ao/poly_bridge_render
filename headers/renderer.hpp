#pragma once

#include <vulkan/vulkan.h>

struct GLFWwindow;
struct Mesh;

class Renderer {
public:
  Renderer() = default; // in .cpp Renderer renderer
  ~Renderer();

  Renderer(const Renderer&) = delete; // disable copy constructor
  Renderer& operator = (const Renderer&) = delete; // disable assiging constructor
  // Renderer first: OK
  // Renderer second(first): copy constructor disabled
  // Renderer third: OK
  // third = first: assign constructor disabled

  void init(GLFWwindow* window);
  void uploadMesh(const Mesh& mesh);
  void drawFrame();
  void shutdown();

private:
  GLFWwindow* window_ = nullptr;

  VkInstance instance_ = VK_NULL_HANDLE;
  VkSurfaceKHR surface_ = VK_NULL_HANDLE;

  VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE; // GPU placeholder
  VkDevice device_ = VK_NULL_HANDLE;

  VkQueue graphicalQueue_ = VK_NULL_HANDLE;
  VkQueue presentQueue_ = VK_NULL_HANDLE;

  uint32_t graphicsFamily_ = 0;
  uint32_t presentFamily_ = 0;

  void createInstance();
  void createSurface();
  void pickPhysicalDevice();
  void createLogicalDevice();
  void createSwapchain();
};



