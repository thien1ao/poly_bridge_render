#include "../headers/renderer.hpp"
#include "../headers/sphere.hpp"
#include "../headers/vk_check.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstddef>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct PushConstants {
  glm::mat4 mvp;
  glm::vec4 color;
  glm::vec4 normalMatrix[3];
};

// static_assert (compilation time)
// assert (runtime also ruins)
static_assert(sizeof(PushConstants) == 128, "push constants must stay within 128 bytes");

const char* const kValidationLayer = "VK_LAYER_KHRONOS_validation";

// +
bool validationLayerAvailable()
{
  uint32_t count = 0;
  vkEnumerateInstanceLayerProperties(&count, nullptr);
  std::vector<VkLayerProperties> layers(count);
  vkEnumerateInstanceLayerProperties(&count, layers.data());

  for (const auto& l : layers) {
    if (std::strcmp(l.layerName, kValidationLayer) == 0) return true;
  }
  return false;
}

// const std::string& path <-- file name
// std::string& <-- no copy instead address
// std::vector<char> output buffer (byte buffer)
//
// open file --> know the size (go the end) --> create byte buffer --> go at the beginning --> read file
std::vector<char> readFile(const std::string& path)
{
  // ate = start at the end, so tellg() gives the file size
  // std::ios:;binary <-- open in binary mode
  std::ifstream file(path, std::ios::ate | std::ios::binary);
  if (!file) {
    throw std::runtime_error("failed to open " + path);
  }

  std::vector<char> buffer(static_cast<size_t>(file.tellg()));
  file.seekg(0); // to the beginning
  // file.read() explicit casting
  // without casting could also work
  file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
  return buffer;
}

// byte code from readFile as address not a copy
VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code)
{
  VkShaderModuleCreateInfo info{};
  info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  info.codeSize = code.size();
  info.pCode    = reinterpret_cast<const uint32_t*>(code.data()); // low lvl cast
  // code.data of rype const char* pCode recieved const uint32_t*
  // const char*
  // b1 b2 b3 b4 b5 b6 b7 <-- char
  // [       ] [        ] <-- uinnt32_t

  VkShaderModule module = VK_NULL_HANDLE;
  checkVk(vkCreateShaderModule(device, &info, nullptr, &module), "vkCreateShaderModule");
  return module;
}

// Z-Buffer +
VkFormat findDepthFormat(VkPhysicalDevice physical)
{
  const VkFormat candidates[] = {
    VK_FORMAT_D32_SFLOAT,
    VK_FORMAT_D32_SFLOAT_S8_UINT,
    VK_FORMAT_D24_UNORM_S8_UINT,
  };

  for (VkFormat f : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(physical, f, &props);
    if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      return f;
    }
  }
  throw std::runtime_error("no supported depth format");
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
    device_.create(instance_, surface_);
    createSwapchain();
    createDepthResources();
    createRenderPass();
    createPipeline();
    createFramebuffers();
    createCommandObjects();
    createSyncObjects();
  }
  catch (...) {
    shutdown();
    throw;
  }
}

// + (fix)
void Renderer::createInstance()
{
  if (!glfwVulkanSupported()) {
    throw std::runtime_error("vulkan is not supported");
  }

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

// Surface creation
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

void Renderer::createSwapchain()
{
  swapchain_.create(device_, surface_, window_);
}

// memory + device + swapchain + depthFormat --> ?
void Renderer::createDepthResources()
{
  VkDevice dev = device_.logical();
  depthFormat_ = findDepthFormat(device_.physical());

  VkExtent2D extent = swapchain_.extent();

  VkImageCreateInfo imageInfo{};
  imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType     = VK_IMAGE_TYPE_2D;
  imageInfo.format        = depthFormat_;
  imageInfo.extent        = { extent.width, extent.height, 1 };
  imageInfo.mipLevels     = 1;
  imageInfo.arrayLayers   = 1;
  imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  checkVk(vkCreateImage(dev, &imageInfo, nullptr, &depthImage_), "vkCreateImage(depth)");

  VkMemoryRequirements req;
  vkGetImageMemoryRequirements(dev, depthImage_, &req);

  VkMemoryAllocateInfo alloc{};
  alloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc.allocationSize  = req.size;
  alloc.memoryTypeIndex = findMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  checkVk(vkAllocateMemory(dev, &alloc, nullptr, &depthMemory_), "vkAllocateMemory(depth)");

  vkBindImageMemory(dev, depthImage_, depthMemory_, 0);

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image    = depthImage_;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format   = depthFormat_;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.layerCount = 1;

  checkVk(vkCreateImageView(dev, &viewInfo, nullptr, &depthView_), "vkCreateImageView(depth)");
}

void Renderer::createRenderPass()
{
  VkAttachmentDescription color{};
  color.format         = swapchain_.format();
  color.samples        = VK_SAMPLE_COUNT_1_BIT;
  color.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
  color.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
  color.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentDescription depth{};
  depth.format         = depthFormat_;
  depth.samples        = VK_SAMPLE_COUNT_1_BIT;
  depth.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
  depth.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference colorRef{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
  VkAttachmentReference depthRef{ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount    = 1;
  subpass.pColorAttachments       = &colorRef;
  subpass.pDepthStencilAttachment = &depthRef;

  // wait until the previous frame is done with the color/depth images
  VkSubpassDependency dependency{};
  dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass    = 0;
  dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                             VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                             VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  VkAttachmentDescription attachments[] = { color, depth };

  VkRenderPassCreateInfo info{};
  info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  info.attachmentCount = 2;
  info.pAttachments    = attachments;
  info.subpassCount    = 1;
  info.pSubpasses      = &subpass;
  info.dependencyCount = 1;
  info.pDependencies   = &dependency;

  checkVk(vkCreateRenderPass(device_.logical(), &info, nullptr, &renderPass_), "vkCreateRenderPass");
}

void Renderer::createPipeline()
{
  VkDevice dev = device_.logical();

  VkShaderModule vert = createShaderModule(dev, readFile(SHADER_DIR "/sphere.vert.spv"));
  VkShaderModule frag = createShaderModule(dev, readFile(SHADER_DIR "/sphere.frag.spv"));

  VkPipelineShaderStageCreateInfo stages[2]{};
  stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
  stages[0].module = vert;
  stages[0].pName  = "main";
  stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
  stages[1].module = frag;
  stages[1].pName  = "main";

  // Vertex = { vec3 position, vec3 normal, vec2 uv }
  VkVertexInputBindingDescription binding{};
  binding.binding   = 0;
  binding.stride    = sizeof(Vertex);
  binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkVertexInputAttributeDescription attributes[3]{};
  attributes[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, position)) };
  attributes[1] = { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, normal)) };
  attributes[2] = { 2, 0, VK_FORMAT_R32G32_SFLOAT,    static_cast<uint32_t>(offsetof(Vertex, uv)) };

  VkPipelineVertexInputStateCreateInfo vertexInput{};
  vertexInput.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInput.vertexBindingDescriptionCount   = 1;
  vertexInput.pVertexBindingDescriptions      = &binding;
  vertexInput.vertexAttributeDescriptionCount = 3;
  vertexInput.pVertexAttributeDescriptions    = attributes;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

  // viewport/scissor are dynamic so the pipeline survives window resizes
  VkPipelineViewportStateCreateInfo viewport{};
  viewport.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport.viewportCount = 1;
  viewport.scissorCount  = 1;

  VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
  VkPipelineDynamicStateCreateInfo dynamic{};
  dynamic.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic.dynamicStateCount = 2;
  dynamic.pDynamicStates    = dynamicStates;

  // sphere triangles are counter-clockwise seen from outside
  VkPipelineRasterizationStateCreateInfo raster{};
  raster.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  raster.polygonMode = VK_POLYGON_MODE_FILL;
  raster.cullMode    = VK_CULL_MODE_BACK_BIT;
  raster.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  raster.lineWidth   = 1.0f;

  VkPipelineMultisampleStateCreateInfo multisample{};
  multisample.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable  = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp   = VK_COMPARE_OP_LESS;

  VkPipelineColorBlendAttachmentState blendAttachment{};
  blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                   VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  VkPipelineColorBlendStateCreateInfo blend{};
  blend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  blend.attachmentCount = 1;
  blend.pAttachments    = &blendAttachment;

  VkPushConstantRange pushRange{};
  pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pushRange.offset     = 0;
  pushRange.size       = sizeof(PushConstants);

  VkPipelineLayoutCreateInfo layoutInfo{};
  layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.pushConstantRangeCount = 1;
  layoutInfo.pPushConstantRanges    = &pushRange;

  checkVk(vkCreatePipelineLayout(dev, &layoutInfo, nullptr, &pipelineLayout_), "vkCreatePipelineLayout");

  VkGraphicsPipelineCreateInfo info{};
  info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  info.stageCount          = 2;
  info.pStages             = stages;
  info.pVertexInputState   = &vertexInput;
  info.pInputAssemblyState = &inputAssembly;
  info.pViewportState      = &viewport;
  info.pRasterizationState = &raster;
  info.pMultisampleState   = &multisample;
  info.pDepthStencilState  = &depthStencil;
  info.pColorBlendState    = &blend;
  info.pDynamicState       = &dynamic;
  info.layout              = pipelineLayout_;
  info.renderPass          = renderPass_;
  info.subpass             = 0;

  VkResult result = vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline_);

  // modules are only needed while the pipeline is built
  vkDestroyShaderModule(dev, vert, nullptr);
  vkDestroyShaderModule(dev, frag, nullptr);

  checkVk(result, "vkCreateGraphicsPipelines");
}

void Renderer::createFramebuffers()
{
  const auto& views = swapchain_.imageViews();
  VkExtent2D extent = swapchain_.extent();

  framebuffers_.resize(views.size(), VK_NULL_HANDLE);

  for (size_t i = 0; i < views.size(); ++i) {
    VkImageView attachments[] = { views[i], depthView_ };

    VkFramebufferCreateInfo info{};
    info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass      = renderPass_;
    info.attachmentCount = 2;
    info.pAttachments    = attachments;
    info.width           = extent.width;
    info.height          = extent.height;
    info.layers          = 1;

    checkVk(vkCreateFramebuffer(device_.logical(), &info, nullptr, &framebuffers_[i]), "vkCreateFramebuffer");
  }
}

void Renderer::createCommandObjects()
{
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // re-record every frame
  poolInfo.queueFamilyIndex = *device_.queueFamilies().graphics;

  checkVk(vkCreateCommandPool(device_.logical(), &poolInfo, nullptr, &commandPool_), "vkCreateCommandPool");

  VkCommandBufferAllocateInfo alloc{};
  alloc.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc.commandPool        = commandPool_;
  alloc.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc.commandBufferCount = kFramesInFlight;

  checkVk(vkAllocateCommandBuffers(device_.logical(), &alloc, commandBuffers_.data()), "vkAllocateCommandBuffers");
}

void Renderer::createSyncObjects()
{
  VkDevice dev = device_.logical();

  VkSemaphoreCreateInfo semInfo{};
  semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  // signaled at start so the first wait in drawFrame doesn't block forever
  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (uint32_t i = 0; i < kFramesInFlight; ++i) {
    checkVk(vkCreateSemaphore(dev, &semInfo, nullptr, &imageAvailable_[i]), "vkCreateSemaphore");
    checkVk(vkCreateFence(dev, &fenceInfo, nullptr, &inFlight_[i]), "vkCreateFence");
  }

  createRenderFinishedSemaphores();
}

void Renderer::createRenderFinishedSemaphores()
{
  VkSemaphoreCreateInfo semInfo{};
  semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  renderFinished_.resize(swapchain_.imageViews().size(), VK_NULL_HANDLE);
  for (auto& sem : renderFinished_) {
    checkVk(vkCreateSemaphore(device_.logical(), &semInfo, nullptr, &sem), "vkCreateSemaphore");
  }
}

uint32_t Renderer::findMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties) const
{
  VkPhysicalDeviceMemoryProperties mem;
  vkGetPhysicalDeviceMemoryProperties(device_.physical(), &mem);

  for (uint32_t i = 0; i < mem.memoryTypeCount; ++i) {
    if ((typeBits & (1u << i)) &&
        (mem.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }
  throw std::runtime_error("no suitable memory type");
}

void Renderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, const void* data,
                            VkBuffer& buffer, VkDeviceMemory& memory)
{
  VkDevice dev = device_.logical();

  VkBufferCreateInfo info{};
  info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  info.size        = size;
  info.usage       = usage;
  info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  checkVk(vkCreateBuffer(dev, &info, nullptr, &buffer), "vkCreateBuffer");

  VkMemoryRequirements req;
  vkGetBufferMemoryRequirements(dev, buffer, &req);

  // host visible + coherent: CPU can write directly, fine for a small mesh
  VkMemoryAllocateInfo alloc{};
  alloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc.allocationSize  = req.size;
  alloc.memoryTypeIndex = findMemoryType(req.memoryTypeBits,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  checkVk(vkAllocateMemory(dev, &alloc, nullptr, &memory), "vkAllocateMemory");
  checkVk(vkBindBufferMemory(dev, buffer, memory, 0), "vkBindBufferMemory");

  void* mapped = nullptr;
  checkVk(vkMapMemory(dev, memory, 0, size, 0, &mapped), "vkMapMemory");
  std::memcpy(mapped, data, static_cast<size_t>(size));
  vkUnmapMemory(dev, memory);
}

uint32_t Renderer::uploadMesh(const Mesh& mesh)
{
  if (mesh.vertices.empty() || mesh.indices.empty()) {
    throw std::runtime_error("cannot upload empty mesh");
  }

  GpuMesh gpu;
  createBuffer(sizeof(Vertex) * mesh.vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
               mesh.vertices.data(), gpu.vertexBuffer, gpu.vertexMemory);
  createBuffer(sizeof(uint32_t) * mesh.indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
               mesh.indices.data(), gpu.indexBuffer, gpu.indexMemory);
  gpu.indexCount = static_cast<uint32_t>(mesh.indices.size());

  meshes_.push_back(gpu);
  return static_cast<uint32_t>(meshes_.size() - 1);
}

void Renderer::addObject(uint32_t meshId, const glm::mat4& model, const glm::vec3& color,
                         const glm::vec3& checkerColor, uint32_t checkerSquares)
{
  if (meshId >= meshes_.size()) {
    throw std::runtime_error("addObject: unknown mesh id");
  }
  objects_.push_back({ meshId, model, color, checkerColor, checkerSquares });
}

void Renderer::recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex)
{
  VkCommandBufferBeginInfo begin{};
  begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  checkVk(vkBeginCommandBuffer(cmd, &begin), "vkBeginCommandBuffer");

  VkExtent2D extent = swapchain_.extent();

  VkClearValue clears[2]{};
  clears[0].color        = { { 0.0f, 0.0f, 0.0f, 1.0f } };
  clears[1].depthStencil = { 1.0f, 0 };

  VkRenderPassBeginInfo pass{};
  pass.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  pass.renderPass        = renderPass_;
  pass.framebuffer       = framebuffers_[imageIndex];
  pass.renderArea.extent = extent;
  pass.clearValueCount   = 2;
  pass.pClearValues      = clears;

  vkCmdBeginRenderPass(cmd, &pass, VK_SUBPASS_CONTENTS_INLINE);

  if (!objects_.empty()) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

    VkViewport viewport{};
    viewport.width    = static_cast<float>(extent.width);
    viewport.height   = static_cast<float>(extent.height);
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{ { 0, 0 }, extent };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // camera in front of the scene, a bit above, looking at the middle between the pillars
    float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);

    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 4.0f, 10.0f), glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    proj[1][1] *= -1.0f; // glm is made for OpenGL, Vulkan's y axis points down

    glm::mat4 viewProj = proj * view;

    for (const SceneObject& obj : objects_) {
      const GpuMesh& mesh = meshes_[obj.meshId];

      VkDeviceSize offset = 0;
      vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.vertexBuffer, &offset);
      vkCmdBindIndexBuffer(cmd, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

      // inverse-transpose keeps normals correct even for scaled objects
      glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(obj.model)));

      PushConstants push{};
      push.mvp   = viewProj * obj.model;
      push.color = glm::vec4(obj.color, static_cast<float>(obj.checkerSquares));
      for (int c = 0; c < 3; ++c) {
        push.normalMatrix[c] = glm::vec4(normalMatrix[c], obj.checkerColor[c]);
      }
      vkCmdPushConstants(cmd, pipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push), &push);

      vkCmdDrawIndexed(cmd, mesh.indexCount, 1, 0, 0, 0);
    }
  }

  vkCmdEndRenderPass(cmd);
  checkVk(vkEndCommandBuffer(cmd), "vkEndCommandBuffer");
}

void Renderer::drawFrame()
{
  if (device_.logical() == VK_NULL_HANDLE) return;

  // minimized window has a 0x0 framebuffer, nothing to draw into
  int width = 0, height = 0;
  glfwGetFramebufferSize(window_, &width, &height);
  if (width == 0 || height == 0) return;

  VkDevice dev = device_.logical();
  uint32_t frame = currentFrame_;

  vkWaitForFences(dev, 1, &inFlight_[frame], VK_TRUE, UINT64_MAX);

  uint32_t imageIndex = 0;
  VkResult result = vkAcquireNextImageKHR(dev, swapchain_.handle(), UINT64_MAX,
                                          imageAvailable_[frame], VK_NULL_HANDLE, &imageIndex);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapchain();
    return;
  }
  if (result != VK_SUBOPTIMAL_KHR) {
    checkVk(result, "vkAcquireNextImageKHR");
  }

  // reset only once we know we will submit work, otherwise the fence never signals
  vkResetFences(dev, 1, &inFlight_[frame]);

  VkCommandBuffer cmd = commandBuffers_[frame];
  vkResetCommandBuffer(cmd, 0);
  recordCommandBuffer(cmd, imageIndex);

  VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  VkSubmitInfo submit{};
  submit.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.waitSemaphoreCount   = 1;
  submit.pWaitSemaphores      = &imageAvailable_[frame];
  submit.pWaitDstStageMask    = &waitStage;
  submit.commandBufferCount   = 1;
  submit.pCommandBuffers      = &cmd;
  submit.signalSemaphoreCount = 1;
  submit.pSignalSemaphores    = &renderFinished_[imageIndex];

  checkVk(vkQueueSubmit(device_.graphicsQueue(), 1, &submit, inFlight_[frame]), "vkQueueSubmit");

  VkSwapchainKHR swapchain = swapchain_.handle();

  VkPresentInfoKHR present{};
  present.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present.waitSemaphoreCount = 1;
  present.pWaitSemaphores    = &renderFinished_[imageIndex];
  present.swapchainCount     = 1;
  present.pSwapchains        = &swapchain;
  present.pImageIndices      = &imageIndex;

  result = vkQueuePresentKHR(device_.presentQueue(), &present);

  currentFrame_ = (currentFrame_ + 1) % kFramesInFlight;

  // some platforms (e.g. Wayland) never report OUT_OF_DATE, so compare sizes too
  VkExtent2D extent = swapchain_.extent();
  bool resized = extent.width  != static_cast<uint32_t>(width) ||
                 extent.height != static_cast<uint32_t>(height);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || resized) {
    recreateSwapchain();
  } else {
    checkVk(result, "vkQueuePresentKHR");
  }
}

void Renderer::recreateSwapchain()
{
  vkDeviceWaitIdle(device_.logical());

  destroySwapchainResources();

  createSwapchain();
  createDepthResources();
  createFramebuffers();
  createRenderFinishedSemaphores();
}

void Renderer::destroySwapchainResources()
{
  VkDevice dev = device_.logical();

  for (VkSemaphore sem : renderFinished_) {
    if (sem != VK_NULL_HANDLE) vkDestroySemaphore(dev, sem, nullptr);
  }
  renderFinished_.clear();

  for (VkFramebuffer fb : framebuffers_) {
    if (fb != VK_NULL_HANDLE) vkDestroyFramebuffer(dev, fb, nullptr);
  }
  framebuffers_.clear();

  if (depthView_ != VK_NULL_HANDLE)   { vkDestroyImageView(dev, depthView_, nullptr); depthView_ = VK_NULL_HANDLE; }
  if (depthImage_ != VK_NULL_HANDLE)  { vkDestroyImage(dev, depthImage_, nullptr);    depthImage_ = VK_NULL_HANDLE; }
  if (depthMemory_ != VK_NULL_HANDLE) { vkFreeMemory(dev, depthMemory_, nullptr);     depthMemory_ = VK_NULL_HANDLE; }

  swapchain_.destroy();
}

void Renderer::destroyMeshes()
{
  VkDevice dev = device_.logical();

  for (GpuMesh& m : meshes_) {
    if (m.vertexBuffer != VK_NULL_HANDLE) vkDestroyBuffer(dev, m.vertexBuffer, nullptr);
    if (m.vertexMemory != VK_NULL_HANDLE) vkFreeMemory(dev, m.vertexMemory, nullptr);
    if (m.indexBuffer != VK_NULL_HANDLE)  vkDestroyBuffer(dev, m.indexBuffer, nullptr);
    if (m.indexMemory != VK_NULL_HANDLE)  vkFreeMemory(dev, m.indexMemory, nullptr);
  }
  meshes_.clear();
  objects_.clear();
}

void Renderer::shutdown()
{
    // reverse order of creation, GPU must be idle first
    if (device_.logical() != VK_NULL_HANDLE) {
        VkDevice dev = device_.logical();
        vkDeviceWaitIdle(dev);

        destroyMeshes();

        for (uint32_t i = 0; i < kFramesInFlight; ++i) {
            if (imageAvailable_[i] != VK_NULL_HANDLE) vkDestroySemaphore(dev, imageAvailable_[i], nullptr);
            if (inFlight_[i] != VK_NULL_HANDLE)       vkDestroyFence(dev, inFlight_[i], nullptr);
            imageAvailable_[i] = VK_NULL_HANDLE;
            inFlight_[i]       = VK_NULL_HANDLE;
        }

        // destroying the pool frees its command buffers too
        if (commandPool_ != VK_NULL_HANDLE) {
            vkDestroyCommandPool(dev, commandPool_, nullptr);
            commandPool_ = VK_NULL_HANDLE;
            commandBuffers_.fill(VK_NULL_HANDLE);
        }

        if (pipeline_ != VK_NULL_HANDLE)       { vkDestroyPipeline(dev, pipeline_, nullptr);             pipeline_ = VK_NULL_HANDLE; }
        if (pipelineLayout_ != VK_NULL_HANDLE) { vkDestroyPipelineLayout(dev, pipelineLayout_, nullptr); pipelineLayout_ = VK_NULL_HANDLE; }

        destroySwapchainResources();

        if (renderPass_ != VK_NULL_HANDLE) { vkDestroyRenderPass(dev, renderPass_, nullptr); renderPass_ = VK_NULL_HANDLE; }
    }

    device_.destroy();

    if (surface_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }

    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }

    currentFrame_ = 0;
    window_ = nullptr;
}
