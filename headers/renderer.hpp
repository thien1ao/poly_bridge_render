#pragma once

#include <vulkan/vulkan.h>

#include "device.hpp"
#include "swapchain.hpp"

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <vector>

struct GLFWwindow;
struct Mesh;

class Renderer {
public:
  Renderer() = default;
  ~Renderer();

  Renderer(const Renderer&) = delete; // disable copy constructor
  Renderer& operator = (const Renderer&) = delete; // disable assiging constructor

  void init(GLFWwindow* window);
  // uploads a mesh to the GPU, returns its id for addObject()
  uint32_t uploadMesh(const Mesh& mesh);

  // one thing to draw: which mesh, where (model matrix) and in which color.
  // checkerSquares > 0 paints a checker pattern of color and checkerColor:
  // checkerSquares squares along v, twice as many along u (around a sphere)
  void addObject(uint32_t meshId, const glm::mat4& model, const glm::vec3& color,
                 const glm::vec3& checkerColor = glm::vec3(0.0f), uint32_t checkerSquares = 0);
  void drawFrame();
  void shutdown();

private:
  // CPU can prepare frame N+1 while GPU still renders frame N
  static constexpr uint32_t kFramesInFlight = 2;

  GLFWwindow* window_ = nullptr;

  VkInstance instance_ = VK_NULL_HANDLE;
  VkSurfaceKHR surface_ = VK_NULL_HANDLE;

  Device device_;       // GPU, logical device and queues
  Swapchain swapchain_;

  // depth buffer so near triangles hide far ones
  VkFormat       depthFormat_ = VK_FORMAT_UNDEFINED;
  VkImage        depthImage_  = VK_NULL_HANDLE;
  VkDeviceMemory depthMemory_ = VK_NULL_HANDLE;
  VkImageView    depthView_   = VK_NULL_HANDLE;

  VkRenderPass     renderPass_     = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
  VkPipeline       pipeline_       = VK_NULL_HANDLE;

  // one framebuffer per swapchain image
  std::vector<VkFramebuffer> framebuffers_;

  VkCommandPool commandPool_ = VK_NULL_HANDLE;
  std::array<VkCommandBuffer, kFramesInFlight> commandBuffers_{};

  // sync: acquire semaphore + fence per frame, render-finished semaphore per swapchain image
  std::array<VkSemaphore, kFramesInFlight> imageAvailable_{};
  std::array<VkFence, kFramesInFlight>     inFlight_{};
  std::vector<VkSemaphore>                 renderFinished_;
  uint32_t currentFrame_ = 0;

  struct GpuMesh {
    VkBuffer       vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
    VkBuffer       indexBuffer  = VK_NULL_HANDLE;
    VkDeviceMemory indexMemory  = VK_NULL_HANDLE;
    uint32_t       indexCount   = 0;
  };

  struct SceneObject {
    uint32_t  meshId;
    glm::mat4 model;
    glm::vec3 color;
    glm::vec3 checkerColor;
    uint32_t  checkerSquares; // 0 = solid color
  };

  // one mesh can be drawn by many objects (e.g. both pillars share a box)
  std::vector<GpuMesh>     meshes_;
  std::vector<SceneObject> objects_;

  void createInstance();
  void createSurface();
  void createSwapchain();
  void createDepthResources();
  void createRenderPass();
  void createPipeline();
  void createFramebuffers();
  void createCommandObjects();
  void createSyncObjects();
  void createRenderFinishedSemaphores();

  void recreateSwapchain();
  void destroySwapchainResources();
  void destroyMeshes();

  void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex);

  uint32_t findMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties) const;
  void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, const void* data,
                    VkBuffer& buffer, VkDeviceMemory& memory);
};
