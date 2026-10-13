#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../headers/renderer.hpp"
#include "../headers/sphere.hpp"

#include <iostream>

int main()
{
    glfwSetErrorCallback([](int code, const char* message) {
        std::cerr << "GLFW error " << code
                  << ": " << message << '\n';
    });

    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);

    if (!glfwInit()) {
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* window = glfwCreateWindow(
        800, 600, "Sphere Renderer", nullptr, nullptr
    );

    if (!window) {
        glfwTerminate();
        return 1;
    }

    while (!glfwWindowShouldClose(window)) {
        glfwWaitEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
