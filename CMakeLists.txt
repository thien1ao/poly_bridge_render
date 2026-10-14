cmake_minimum_required(VERSION 3.16)
project(MyVulkanEngine LANGUAGES CXX)

find_package(Vulkan REQUIRED)
find_package(glfw3 3.4 REQUIRED)

add_executable(my_engine
  src/main.cpp
  src/window.cpp
  src/device.cpp
  src/swapchain.cpp
  src/sphere.cpp
  src/box.cpp
  src/renderer.cpp
)

# compile GLSL shaders to SPIR-V next to the executable
find_program(GLSLC glslc)
if(NOT GLSLC)
  message(FATAL_ERROR "glslc not found (install shaderc or the Vulkan SDK)")
endif()

set(SHADER_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/sphere.vert
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/sphere.frag
)
set(SHADER_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)

foreach(src ${SHADER_SOURCES})
  get_filename_component(name ${src} NAME)
  set(out ${SHADER_OUT_DIR}/${name}.spv)
  add_custom_command(
    OUTPUT ${out}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUT_DIR}
    COMMAND ${GLSLC} ${src} -o ${out}
    DEPENDS ${src}
    VERBATIM
  )
  list(APPEND SPIRV_FILES ${out})
endforeach()

add_custom_target(shaders DEPENDS ${SPIRV_FILES})
add_dependencies(my_engine shaders)

target_compile_definitions(my_engine PRIVATE
  GLM_FORCE_DEPTH_ZERO_TO_ONE
  SHADER_DIR="${SHADER_OUT_DIR}"
)

target_include_directories(my_engine PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/headers"
)

target_compile_features(my_engine PRIVATE cxx_std_17)

target_link_libraries(my_engine PRIVATE
    glfw
    Vulkan::Vulkan
)
