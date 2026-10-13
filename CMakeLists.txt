cmake_minimum_required(VERSION 3.16)
project(MyVulkanEngine LANGUAGES CXX)

find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)

add_executable(my_engine
  src/main.cpp
  src/sphere.cpp
  src/renderer.cpp
)

target_compile_features(my_engine GLM_FORCE_DEPTH_ZERO_TO_ONE)

target_include_directories(my_engine PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/headers"
)

target_compile_features(my_engine PRIVATE cxx_std_17)

target_link_libraries(my_engine PRIVATE
    glfw
    Vulkan::Vulkan
)
