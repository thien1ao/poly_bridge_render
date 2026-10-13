#pragma once

#include <vulkan/vulkan.h>

struct GLFWwindow;
struct Mesh;

class Renderer {
public:
  Renderer() = default;
  ~Renderer();

  // Renderer a
  // a.init(window)
  // Renderer b = a
  // compilation error > runtime error
  Renderer(const Renderer&) = delete;
  Renderer& operator = (const Renderer&) = delete;

  // incapsulate main pipeline
  void init(GLFWwindow* window);
  void uploadMesh(const Mesh& mesh);
  void drawFrame();
  void shutdown();

private:
  GLFWwindow* window_ = nullptr;

  VkInstance instance_ = VK_NULL_HANDLE;
  VkSurfaceKHR surface_ = VK_NULL_HANDLE;

  VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;

  VkQueue graphicalQueue_ = VK_NULL_HANDLE;
  VkQueue presentQueue_ = VK_NULL_HANDLE;

  uint32_t graphicsFamily_ = 0;
  uint32_t presentFamily_ = 0;

  void createInstance();
  void createSurface();
  void pickPhysicalDevice();
  void createLogicalDevice();
};



